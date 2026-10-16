#include "utils/hako_share/hako_runtime_lifecycle.hpp"

#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_share/impl/hako_process_identity.hpp"
#include "utils/hako_share/hako_master_lock.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>

namespace {

using hako::utils::lifecycle::RuntimeLifecycleHealth;

constexpr uint64_t LIFECYCLE_MAGIC = 0x48414B4F4C494645ULL; // HAKOLIFE
constexpr uint32_t LIFECYCLE_VERSION = 1;
constexpr const char *GATE_NAME = "runtime-lifecycle-v1.gate";
constexpr const char *MARKER_NAME = "runtime-lifecycle-v1.bin";
constexpr const char *LEASE_PREFIX = "runtime-participant-v1-";
constexpr const char *LEASE_SUFFIX = ".lease";

struct LifecycleMarkerV1 {
    uint64_t magic = LIFECYCLE_MAGIC;
    uint32_t version = LIFECYCLE_VERSION;
    uint32_t size = 32;
    uint64_t generation = 0;
    uint64_t checksum = 0;
};

uint64_t marker_checksum(const LifecycleMarkerV1& marker)
{
    LifecycleMarkerV1 copy = marker;
    copy.checksum = 0;
    const auto *bytes = reinterpret_cast<const unsigned char*>(&copy);
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < sizeof(copy); ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

bool safe_regular_file(const std::filesystem::path& path)
{
    std::error_code error;
    return !std::filesystem::is_symlink(path, error) && !error &&
           std::filesystem::is_regular_file(path, error) && !error;
}

bool read_marker(const std::filesystem::path& path, LifecycleMarkerV1 *marker,
                 std::string *reason)
{
    if (!safe_regular_file(path)) {
        *reason = "runtime lifecycle marker is missing or is not a regular file";
        return false;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream.read(reinterpret_cast<char*>(marker), sizeof(*marker))) {
        *reason = "runtime lifecycle marker cannot be read";
        return false;
    }
    if (marker->magic != LIFECYCLE_MAGIC ||
        marker->version != LIFECYCLE_VERSION ||
        marker->size != sizeof(*marker) ||
        marker->generation == 0 ||
        marker->checksum != marker_checksum(*marker)) {
        *reason = "runtime lifecycle marker is incompatible or corrupt";
        return false;
    }
    return true;
}

bool write_marker(const std::filesystem::path& path, uint64_t generation,
                  std::string *reason)
{
    std::error_code error;
    if (std::filesystem::exists(path, error) &&
        (!safe_regular_file(path) || error)) {
        *reason = "runtime lifecycle marker path is unsafe";
        return false;
    }
    LifecycleMarkerV1 marker;
    marker.generation = generation;
    marker.checksum = marker_checksum(marker);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(&marker), sizeof(marker));
    stream.flush();
    if (!stream) {
        *reason = "runtime lifecycle marker cannot be written";
        return false;
    }
    return true;
}

uint64_t new_generation()
{
    HakoProcessIdentity identity;
    const uint64_t process = hako_current_process_identity(&identity)
        ? identity.start_token ^ static_cast<uint64_t>(identity.pid)
        : 0;
    const uint64_t clock = static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    const uint64_t value = process ^ clock ^ 0x9E3779B97F4A7C15ULL;
    return value == 0 ? 1 : value;
}

bool runtime_artifacts_exist(const std::filesystem::path& root)
{
    const std::regex mmap_name("mmap-0x[0-9a-fA-F]+\\.bin");
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(root, error)) {
        const std::string name = entry.path().filename().string();
        if (std::regex_match(name, mmap_name) || name == "flock.bin" ||
            name == "pdu_init.lock" || name == "pro_init.lock" ||
            name.rfind(LEASE_PREFIX, 0) == 0) {
            return true;
        }
    }
    return false;
}

std::vector<std::filesystem::path> lease_paths(
    const std::filesystem::path& root, std::string *reason)
{
    std::vector<std::filesystem::path> paths;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(root, error)) {
        const std::string name = entry.path().filename().string();
        if (name.rfind(LEASE_PREFIX, 0) == 0 &&
            name.size() > std::char_traits<char>::length(LEASE_PREFIX) +
                              std::char_traits<char>::length(LEASE_SUFFIX) &&
            name.compare(name.size() - std::char_traits<char>::length(LEASE_SUFFIX),
                         std::char_traits<char>::length(LEASE_SUFFIX),
                         LEASE_SUFFIX) == 0) {
            if (!safe_regular_file(entry.path())) {
                *reason = "runtime participant lease path is unsafe: " +
                          entry.path().string();
                return {};
            }
            paths.push_back(entry.path());
        }
    }
    if (error) {
        *reason = "runtime participant leases cannot be enumerated";
        return {};
    }
    return paths;
}

RuntimeLifecycleHealth inspect_leases(
    const std::filesystem::path& root,
    std::vector<std::string> *inactive,
    std::string *reason)
{
    std::string enumerate_reason;
    const auto paths = lease_paths(root, &enumerate_reason);
    if (!enumerate_reason.empty()) {
        *reason = enumerate_reason;
        return RuntimeLifecycleHealth::Unknown;
    }
    for (const auto& path : paths) {
        std::string value = path.string();
        HakoFlockObjectType *handle = hako_flock_open(value);
        if (handle == nullptr) {
            *reason = "runtime participant lease cannot be opened: " + value;
            return RuntimeLifecycleHealth::Unknown;
        }
        const int acquired = hako_flock_try_acquire(handle);
        if (acquired == 1) {
            hako_flock_destroy(handle);
            *reason = "a runtime participant is still attached: " + value;
            return RuntimeLifecycleHealth::Busy;
        }
        if (acquired != 0) {
            hako_flock_destroy(handle);
            *reason = "runtime participant lease cannot be inspected: " + value;
            return RuntimeLifecycleHealth::Unknown;
        }
        hako_flock_release(handle);
        hako_flock_destroy(handle);
        inactive->push_back(value);
    }
    return RuntimeLifecycleHealth::Ok;
}

bool validate_master_generation(
    const std::filesystem::path& root, uint64_t generation, std::string *reason)
{
    const auto master_path = root / "mmap-0xff.bin";
    std::error_code error;
    if (!std::filesystem::exists(master_path, error)) {
        return !error;
    }
    if (!safe_regular_file(master_path)) {
        *reason = "master memory path is unsafe";
        return false;
    }
    std::ifstream stream(master_path, std::ios::binary);
    hako::utils::SharedMemoryMetaDataType metadata{};
    if (!stream.read(reinterpret_cast<char*>(&metadata), sizeof(metadata))) {
        *reason = "master memory header cannot be read";
        return false;
    }
    if (metadata.magic != HAKO_SHM_MAGIC ||
        metadata.version != HAKO_SHM_LAYOUT_VERSION ||
        metadata.lifecycle_generation != generation) {
        *reason = "master memory is legacy, mixed, or belongs to another lifecycle generation";
        return false;
    }
    return true;
}

class GateLock {
public:
    ~GateLock() { release(); }

    RuntimeLifecycleHealth acquire(const std::filesystem::path& root,
                                   bool nonblocking,
                                   std::string *reason)
    {
        std::error_code error;
        std::filesystem::create_directories(root, error);
        if (error || !std::filesystem::is_directory(root, error) || error) {
            *reason = "core_mmap_path cannot be prepared for lifecycle locking";
            return RuntimeLifecycleHealth::Unknown;
        }
        path_ = root / GATE_NAME;
        if (std::filesystem::exists(path_, error) &&
            (!safe_regular_file(path_) || error)) {
            *reason = "runtime lifecycle gate path is unsafe";
            return RuntimeLifecycleHealth::Unknown;
        }
        std::string path = path_.string();
        handle_ = hako_flock_create(path, 1);
        if (handle_ == nullptr) {
            *reason = "runtime lifecycle gate cannot be opened";
            return RuntimeLifecycleHealth::Unknown;
        }
        if (nonblocking) {
            const int result = hako_flock_try_acquire(handle_);
            if (result == 1) {
                release();
                *reason = "runtime lifecycle gate is busy";
                return RuntimeLifecycleHealth::Busy;
            }
            if (result != 0) {
                release();
                *reason = "runtime lifecycle gate cannot be locked";
                return RuntimeLifecycleHealth::Unknown;
            }
        }
        else {
            try {
                hako_flock_acquire(handle_);
            }
            catch (const std::exception& error) {
                release();
                *reason = error.what();
                return RuntimeLifecycleHealth::Unknown;
            }
        }
        acquired_ = true;
        return RuntimeLifecycleHealth::Ok;
    }

    void release() noexcept
    {
        if (handle_ == nullptr) {
            return;
        }
        if (acquired_) {
            try { hako_flock_release(handle_); } catch (...) {}
        }
        hako_flock_destroy(handle_);
        handle_ = nullptr;
        acquired_ = false;
    }

    HakoFlockObjectType *detach() noexcept
    {
        acquired_ = false;
        auto *value = handle_;
        handle_ = nullptr;
        return value;
    }

private:
    HakoFlockObjectType *handle_ = nullptr;
    bool acquired_ = false;
    std::filesystem::path path_;
};

} // namespace

namespace hako::utils::lifecycle {

struct RuntimeParticipantLease::Impl {
    HakoFlockObjectType *lease = nullptr;
    uint64_t generation = 0;
    std::string path;
};

RuntimeParticipantLease::RuntimeParticipantLease() : impl_(new Impl) {}

RuntimeParticipantLease::~RuntimeParticipantLease()
{
    if (impl_->lease != nullptr) {
        try { hako_flock_release(impl_->lease); } catch (...) {}
        hako_flock_destroy(impl_->lease);
    }
}

void RuntimeParticipantLease::join(const std::string& root_value,
                                   bool create_session)
{
    if (impl_->lease != nullptr) {
        return;
    }
    const std::filesystem::path root(root_value);
    std::string reason;
    GateLock gate;
    const auto gate_health = gate.acquire(root, false, &reason);
    if (gate_health != RuntimeLifecycleHealth::Ok) {
        throw RuntimeLifecycleError(gate_health, reason);
    }

    const auto marker_path = root / MARKER_NAME;
    LifecycleMarkerV1 marker;
    if (!std::filesystem::exists(marker_path)) {
        if (!create_session || runtime_artifacts_exist(root)) {
            throw RuntimeLifecycleError(
                RuntimeLifecycleHealth::Unknown,
                "runtime lifecycle marker is absent while legacy or existing artifacts remain");
        }
        marker.generation = new_generation();
        if (!write_marker(marker_path, marker.generation, &reason)) {
            throw RuntimeLifecycleError(RuntimeLifecycleHealth::Unknown, reason);
        }
    }
    else if (!read_marker(marker_path, &marker, &reason)) {
        throw RuntimeLifecycleError(RuntimeLifecycleHealth::Unknown, reason);
    }

    if (create_session) {
        std::vector<std::string> inactive;
        const auto health = inspect_leases(root, &inactive, &reason);
        if (health != RuntimeLifecycleHealth::Ok) {
            throw RuntimeLifecycleError(health, reason);
        }
        if (!validate_master_generation(root, marker.generation, &reason)) {
            throw RuntimeLifecycleError(RuntimeLifecycleHealth::Unknown, reason);
        }
        const auto flock_path = root / "flock.bin";
        if (std::filesystem::exists(flock_path)) {
            using namespace hako::utils::sem::flock;
            const MasterLockInspection lock = inspect_master_lock(0, false);
            if (lock.health != MasterLockHealth::Ok ||
                lock.state != MasterLockState::Unlocked ||
                lock.owner_pid != 0 || lock.owner_start_token != 0) {
                throw RuntimeLifecycleError(
                    lock.health == MasterLockHealth::Unknown
                        ? RuntimeLifecycleHealth::Unknown
                        : RuntimeLifecycleHealth::Busy,
                    "existing runtime is not cleanly quiescent: " + lock.reason);
            }
        }
        for (const auto& inactive_path : inactive) {
            std::error_code ignored;
            std::filesystem::remove(inactive_path, ignored);
        }
        marker.generation = new_generation();
        if (!write_marker(marker_path, marker.generation, &reason)) {
            throw RuntimeLifecycleError(RuntimeLifecycleHealth::Unknown, reason);
        }
    }

    HakoProcessIdentity identity;
    if (!hako_current_process_identity(&identity)) {
        throw RuntimeLifecycleError(
            RuntimeLifecycleHealth::Unknown,
            "current process identity cannot be captured for participant lease");
    }
    static std::atomic<uint64_t> sequence{0};
    for (uint64_t attempt = 0; attempt < 1000; ++attempt) {
        const uint64_t suffix = sequence.fetch_add(1) + 1;
        const auto lease_path = root /
            (std::string(LEASE_PREFIX) + std::to_string(identity.pid) + "-" +
             std::to_string(identity.start_token) + "-" +
             std::to_string(suffix) + LEASE_SUFFIX);
        std::error_code lease_error;
        if (std::filesystem::exists(lease_path, lease_error) &&
            (!safe_regular_file(lease_path) || lease_error)) {
            throw RuntimeLifecycleError(
                RuntimeLifecycleHealth::Unknown,
                "runtime participant lease path is unsafe: " +
                    lease_path.string());
        }
        std::string lease_value = lease_path.string();
        HakoFlockObjectType *lease = hako_flock_create(lease_value, 1);
        if (lease == nullptr) {
            continue;
        }
        const int acquired = hako_flock_try_acquire(lease);
        if (acquired == 0) {
            impl_->lease = lease;
            impl_->path = lease_value;
            impl_->generation = marker.generation;
            return;
        }
        hako_flock_destroy(lease);
    }
    throw RuntimeLifecycleError(
        RuntimeLifecycleHealth::Unknown,
        "a unique runtime participant lease cannot be acquired");
}

uint64_t RuntimeParticipantLease::generation() const noexcept
{
    return impl_->generation;
}

bool RuntimeParticipantLease::joined() const noexcept
{
    return impl_->lease != nullptr;
}

struct RuntimeRecoveryGuard::Impl {
    HakoFlockObjectType *gate = nullptr;
    uint64_t generation = 0;
    std::vector<std::string> inactive;
};

RuntimeRecoveryGuard::RuntimeRecoveryGuard() : impl_(new Impl) {}
RuntimeRecoveryGuard::~RuntimeRecoveryGuard() { release(); }

RuntimeLifecycleHealth RuntimeRecoveryGuard::acquire(
    const std::string& root_value, std::string *reason)
{
    release();
    const std::filesystem::path root(root_value);
    GateLock gate;
    auto health = gate.acquire(root, true, reason);
    if (health != RuntimeLifecycleHealth::Ok) {
        return health;
    }
    LifecycleMarkerV1 marker;
    if (!read_marker(root / MARKER_NAME, &marker, reason)) {
        return RuntimeLifecycleHealth::Unknown;
    }
    std::vector<std::string> inactive;
    health = inspect_leases(root, &inactive, reason);
    if (health != RuntimeLifecycleHealth::Ok) {
        return health;
    }
    if (!validate_master_generation(root, marker.generation, reason)) {
        return RuntimeLifecycleHealth::Unknown;
    }
    impl_->gate = gate.detach();
    impl_->generation = marker.generation;
    impl_->inactive = std::move(inactive);
    return RuntimeLifecycleHealth::Ok;
}

void RuntimeRecoveryGuard::release() noexcept
{
    if (impl_->gate != nullptr) {
        try { hako_flock_release(impl_->gate); } catch (...) {}
        hako_flock_destroy(impl_->gate);
        impl_->gate = nullptr;
    }
    impl_->generation = 0;
    impl_->inactive.clear();
}

bool RuntimeRecoveryGuard::acquired() const noexcept { return impl_->gate != nullptr; }
uint64_t RuntimeRecoveryGuard::generation() const noexcept { return impl_->generation; }
const std::vector<std::string>& RuntimeRecoveryGuard::inactive_leases() const noexcept
{
    return impl_->inactive;
}

const char* runtime_lifecycle_health_name(RuntimeLifecycleHealth health) noexcept
{
    switch (health) {
    case RuntimeLifecycleHealth::Ok: return "OK";
    case RuntimeLifecycleHealth::Busy: return "BUSY";
    case RuntimeLifecycleHealth::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace hako::utils::lifecycle
