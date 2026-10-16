#include "utils/hako_share/hako_sem_flock.hpp"
#include "utils/hako_share/hako_master_lock.hpp"
#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_share/impl/hako_flock_wait.hpp"
#include "utils/hako_share/impl/hako_process_identity.hpp"
#include "utils/hako_config_loader.hpp"
#include "utils/hako_assert.hpp"

#include <chrono>
#include <cstdio>
#include <mutex>
#include <thread>

#define HAKO_SEM_INX_MASTER 0
#define HAKO_SEM_INX_ASSETS 1

namespace {

constexpr uint32_t MASTER_LOCK_MAGIC = 0x484B464CU;
constexpr uint16_t MASTER_LOCK_VERSION = 1;
constexpr uint64_t MASTER_LOCK_METADATA_OFFSET =
    static_cast<uint64_t>(sizeof(int)) * HAKO_CONFIG_MAX_SEM;
constexpr uint32_t FLOCK_INITIALIZATION_WAIT_MS = 2000;

struct MasterLockMetadataV1 {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t state;
    uint32_t reserved;
    int64_t owner_pid;
    uint64_t owner_start_token;
    uint64_t generation;
    uint64_t checksum;
};

static_assert(sizeof(MasterLockMetadataV1) == 48,
              "master lock metadata layout must remain stable");
constexpr uint64_t MASTER_LOCK_FLOCK_SIZE =
    MASTER_LOCK_METADATA_OFFSET + sizeof(MasterLockMetadataV1);

std::mutex local_mutex;
thread_local HakoFlockObjectType *flock_handle = nullptr;

uint64_t metadata_checksum(const MasterLockMetadataV1& metadata)
{
    MasterLockMetadataV1 copy = metadata;
    copy.checksum = 0;
    const auto *bytes = reinterpret_cast<const unsigned char*>(&copy);
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < sizeof(copy); ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string flock_path()
{
    HakoConfigType config;
    hako_config_load(config);
    if (config.param == nullptr) {
        return "./flock";
    }
    const std::string core_mmap_path = config.param["core_mmap_path"];
    return core_mmap_path + "/flock.bin";
}

void ensure_flock_loaded()
{
    if (flock_handle != nullptr) {
        return;
    }
    std::string filepath = flock_path();
    flock_handle = hako_flock_open(filepath);
    HAKO_ASSERT(flock_handle != nullptr);
}

bool read_metadata(HakoFlockObjectType *handle, MasterLockMetadataV1 *metadata)
{
    if (hako_flock_size(handle) < MASTER_LOCK_METADATA_OFFSET + sizeof(*metadata)) {
        return false;
    }
    return hako_flock_read_bytes(
        handle, MASTER_LOCK_METADATA_OFFSET, metadata, sizeof(*metadata));
}

bool write_metadata(HakoFlockObjectType *handle, MasterLockMetadataV1 *metadata)
{
    metadata->checksum = metadata_checksum(*metadata);
    return hako_flock_write_bytes(
        handle, MASTER_LOCK_METADATA_OFFSET, metadata, sizeof(*metadata));
}

MasterLockMetadataV1 new_metadata()
{
    MasterLockMetadataV1 metadata{};
    metadata.magic = MASTER_LOCK_MAGIC;
    metadata.version = MASTER_LOCK_VERSION;
    metadata.size = sizeof(metadata);
    metadata.state = static_cast<uint32_t>(
        hako::utils::sem::flock::MasterLockState::Unlocked);
    metadata.checksum = metadata_checksum(metadata);
    return metadata;
}

hako::utils::sem::flock::MasterLockInspection inspect_locked(
    HakoFlockObjectType *handle)
{
    using namespace hako::utils::sem::flock;
    MasterLockInspection result{};
    if (!hako_flock_read_bytes(handle, 0, &result.counter, sizeof(result.counter))) {
        result.reason = "master lock counter cannot be read";
        return result;
    }
    MasterLockMetadataV1 metadata{};
    if (!read_metadata(handle, &metadata)) {
        result.reason = "master lock owner metadata is missing (legacy flock.bin)";
        return result;
    }
    if (metadata.magic != MASTER_LOCK_MAGIC ||
        metadata.version != MASTER_LOCK_VERSION ||
        metadata.size != sizeof(metadata)) {
        result.reason = "master lock owner metadata version is incompatible";
        return result;
    }
    if (metadata.checksum != metadata_checksum(metadata)) {
        result.reason = "master lock owner metadata checksum is invalid";
        return result;
    }
    if (metadata.state > static_cast<uint32_t>(MasterLockState::Releasing)) {
        result.reason = "master lock owner metadata state is invalid";
        return result;
    }
    result.state = static_cast<MasterLockState>(metadata.state);
    result.owner_pid = metadata.owner_pid;
    result.owner_start_token = metadata.owner_start_token;
    result.generation = metadata.generation;

    if (result.state == MasterLockState::Unlocked) {
        if (result.counter == 1 && result.owner_pid == 0 &&
            result.owner_start_token == 0) {
            result.health = MasterLockHealth::Ok;
            result.reason = "master lock is unlocked";
        }
        else {
            result.reason = "unlocked master lock metadata is inconsistent";
        }
        return result;
    }
    const HakoProcessIdentityStatus owner_status = hako_check_process_identity(
        {result.owner_pid, result.owner_start_token});
    if (owner_status == HakoProcessIdentityStatus::Dead) {
        result.health = MasterLockHealth::Abort;
        result.reason = "master lock owner process is no longer alive";
        return result;
    }
    if (owner_status == HakoProcessIdentityStatus::Unknown) {
        result.reason = "master lock owner identity cannot be verified";
        return result;
    }
    if (result.state == MasterLockState::Locked && result.counter == 0) {
        result.health = MasterLockHealth::Ok;
        result.reason = "master lock is held by a live process";
        return result;
    }
    result.reason = "master lock transition was left incomplete";
    return result;
}

[[noreturn]] void throw_lock_health(
    const hako::utils::sem::flock::MasterLockInspection& inspection)
{
    using namespace hako::utils::sem::flock;
    std::fprintf(stderr,
        "HAKONIWA_FAIL_STOP health=%s owner_pid=%lld generation=%llu reason=%s\n",
        master_lock_health_name(inspection.health),
        static_cast<long long>(inspection.owner_pid),
        static_cast<unsigned long long>(inspection.generation),
        inspection.reason.c_str());
    throw MasterLockStateError(inspection.health, inspection.reason);
}

void initialize_new_flock(HakoFlockObjectType *handle)
{
    hako_flock_acquire(handle);
    for (int i = 0; i < HAKO_CONFIG_MAX_SEM; ++i) {
        const int value = i == HAKO_SEM_INX_MASTER ? 1 : 0;
        if (!hako_flock_write_bytes(
                handle, static_cast<uint64_t>(i) * sizeof(value),
                &value, sizeof(value))) {
            hako_flock_release(handle);
            throw hako::utils::sem::flock::MasterLockStateError(
                hako::utils::sem::flock::MasterLockHealth::Unknown,
                "failed to initialize semaphore counters");
        }
    }
    MasterLockMetadataV1 metadata = new_metadata();
    if (!write_metadata(handle, &metadata)) {
        hako_flock_release(handle);
        throw hako::utils::sem::flock::MasterLockStateError(
            hako::utils::sem::flock::MasterLockHealth::Unknown,
            "failed to initialize master lock owner metadata");
    }
    hako_flock_release(handle);
}

void wait_for_existing_flock_initialization(HakoFlockObjectType *handle)
{
    const uint64_t initial_size = hako_flock_size(handle);
    if (initial_size == MASTER_LOCK_METADATA_OFFSET) {
        // Exact legacy layout: do not reinterpret it as an in-progress v1
        // file and do not delay or modify it.
        return;
    }
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(FLOCK_INITIALIZATION_WAIT_MS);
    while (std::chrono::steady_clock::now() < deadline) {
        const int lock_result = hako_flock_try_acquire(handle);
        if (lock_result < 0) {
            return;
        }
        if (lock_result == 0) {
            MasterLockMetadataV1 metadata{};
            const bool initialized = read_metadata(handle, &metadata) &&
                metadata.magic == MASTER_LOCK_MAGIC &&
                metadata.version == MASTER_LOCK_VERSION &&
                metadata.size == sizeof(metadata) &&
                metadata.checksum == metadata_checksum(metadata);
            hako_flock_release(handle);
            if (initialized) {
                return;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    // A legacy or interrupted flock.bin must remain untouched.  Its first
    // logical lock operation will report UNKNOWN through inspect_locked().
}

void semaphore_down(int index)
{
    int value = 0;
    const uint64_t started_at_ms = hako_flock_wait_now_ms();
    std::unique_lock<std::mutex> lock(local_mutex);
    ensure_flock_loaded();
    while (true) {
        hako_flock_wait_checkpoint(started_at_ms);
        hako_flock_acquire(flock_handle);
        hako_flock_read(flock_handle, index, &value);
        if (value > 0) {
            hako_flock_write(flock_handle, index, --value);
            hako_flock_release(flock_handle);
            return;
        }
        hako_flock_release(flock_handle);
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        hako_flock_wait_checkpoint(started_at_ms);
        lock.lock();
    }
}

void semaphore_up(int index)
{
    int value = 0;
    std::unique_lock<std::mutex> lock(local_mutex);
    ensure_flock_loaded();
    hako_flock_acquire(flock_handle);
    hako_flock_read(flock_handle, index, &value);
    hako_flock_write(flock_handle, index, ++value);
    hako_flock_release(flock_handle);
}

} // namespace

const char* hako::utils::sem::flock::master_lock_health_name(
    MasterLockHealth health)
{
    switch (health) {
    case MasterLockHealth::Ok: return "OK";
    case MasterLockHealth::Abort: return "ABORT";
    case MasterLockHealth::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

const char* hako::utils::sem::flock::master_lock_state_name(
    MasterLockState state)
{
    switch (state) {
    case MasterLockState::Unlocked: return "UNLOCKED";
    case MasterLockState::Acquiring: return "ACQUIRING";
    case MasterLockState::Locked: return "LOCKED";
    case MasterLockState::Releasing: return "RELEASING";
    }
    return "INVALID";
}

hako::utils::sem::flock::MasterLockInspection
hako::utils::sem::flock::inspect_master_lock(uint32_t timeout_ms, bool force_wait)
{
    MasterLockInspection result{};
    std::string filepath;
    try {
        filepath = flock_path();
    }
    catch (const std::exception& e) {
        result.reason = e.what();
        return result;
    }
    HakoFlockObjectType *handle = hako_flock_open(filepath);
    if (handle == nullptr) {
        result.reason = "flock.bin cannot be opened";
        return result;
    }
    try {
        if (force_wait) {
            HakoFlockWaitScope scope({timeout_ms, nullptr});
            hako_flock_acquire(handle);
        }
        else {
            const int lock_result = hako_flock_try_acquire(handle);
            if (lock_result != 0) {
                result.reason = lock_result > 0
                    ? "OS file lock is busy"
                    : "OS file lock inspection failed";
                hako_flock_destroy(handle);
                return result;
            }
        }
        result = inspect_locked(handle);
        hako_flock_release(handle);
    }
    catch (const std::exception& e) {
        result.reason = e.what();
    }
    hako_flock_destroy(handle);
    return result;
}

int32_t hako::utils::sem::flock::create(int32_t key)
{
    std::string filepath = flock_path();
    if (flock_handle != nullptr) {
        hako_flock_destroy(flock_handle);
        flock_handle = nullptr;
    }
    flock_handle = hako_flock_create(filepath, MASTER_LOCK_FLOCK_SIZE);
    if (flock_handle == nullptr) {
        return -1;
    }
    if (flock_handle->created) {
        initialize_new_flock(flock_handle);
    }
    else {
        // A concurrent creator publishes flock.bin before it can commit the
        // versioned metadata.  Wait only here (never in the default check
        // path) so normal simultaneous startup does not mistake that short
        // window for a legacy/corrupt session.  The wait is bounded and does
        // not modify legacy files.
        wait_for_existing_flock_initialization(flock_handle);
    }
    return key;
}

void hako::utils::sem::flock::destroy(int32_t sem_id)
{
    HAKO_ASSERT(sem_id >= 0);
    if (flock_handle != nullptr) {
        hako_flock_destroy(flock_handle);
        flock_handle = nullptr;
    }
}

void hako::utils::sem::flock::asset_down(int32_t sem_id, int32_t asset_id)
{
    HAKO_ASSERT(sem_id >= 0);
    semaphore_down(HAKO_SEM_INX_ASSETS + asset_id);
}

void hako::utils::sem::flock::asset_up(int32_t sem_id, int32_t asset_id)
{
    HAKO_ASSERT(sem_id >= 0);
    semaphore_up(HAKO_SEM_INX_ASSETS + asset_id);
}

void hako::utils::sem::flock::master_lock(int32_t sem_id)
{
    HAKO_ASSERT(sem_id >= 0);
    HakoProcessIdentity owner{};
    if (!hako_current_process_identity(&owner)) {
        MasterLockInspection unknown{};
        unknown.reason = "current process identity cannot be determined";
        throw_lock_health(unknown);
    }
    const uint64_t started_at_ms = hako_flock_wait_now_ms();
    std::unique_lock<std::mutex> local_lock(local_mutex);
    ensure_flock_loaded();
    while (true) {
        hako_flock_wait_checkpoint(started_at_ms);
        hako_flock_acquire(flock_handle);
        MasterLockInspection inspection = inspect_locked(flock_handle);
        if (inspection.health != MasterLockHealth::Ok) {
            hako_flock_release(flock_handle);
            throw_lock_health(inspection);
        }
        if (inspection.state == MasterLockState::Unlocked && inspection.counter == 1) {
            MasterLockMetadataV1 metadata{};
            if (!read_metadata(flock_handle, &metadata)) {
                hako_flock_release(flock_handle);
                inspection.health = MasterLockHealth::Unknown;
                inspection.reason = "master lock metadata disappeared during acquisition";
                throw_lock_health(inspection);
            }
            metadata.state = static_cast<uint32_t>(MasterLockState::Acquiring);
            metadata.owner_pid = owner.pid;
            metadata.owner_start_token = owner.start_token;
            ++metadata.generation;
            if (!write_metadata(flock_handle, &metadata)) {
                hako_flock_release(flock_handle);
                inspection.health = MasterLockHealth::Unknown;
                inspection.reason = "failed to record master lock acquisition";
                throw_lock_health(inspection);
            }
            const int locked_counter = 0;
            if (!hako_flock_write_bytes(
                    flock_handle, 0, &locked_counter, sizeof(locked_counter))) {
                hako_flock_release(flock_handle);
                inspection.health = MasterLockHealth::Abort;
                inspection.owner_pid = owner.pid;
                inspection.generation = metadata.generation;
                inspection.reason = "master lock counter acquisition failed";
                throw_lock_health(inspection);
            }
            metadata.state = static_cast<uint32_t>(MasterLockState::Locked);
            if (!write_metadata(flock_handle, &metadata)) {
                hako_flock_release(flock_handle);
                inspection.health = MasterLockHealth::Abort;
                inspection.owner_pid = owner.pid;
                inspection.generation = metadata.generation;
                inspection.reason = "master lock acquisition metadata was not committed";
                throw_lock_health(inspection);
            }
            hako_flock_release(flock_handle);
            return;
        }
        hako_flock_release(flock_handle);
        local_lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        hako_flock_wait_checkpoint(started_at_ms);
        local_lock.lock();
    }
}

void hako::utils::sem::flock::master_unlock(int32_t sem_id)
{
    HAKO_ASSERT(sem_id >= 0);
    HakoProcessIdentity owner{};
    if (!hako_current_process_identity(&owner)) {
        MasterLockInspection unknown{};
        unknown.reason = "current process identity cannot be determined";
        throw_lock_health(unknown);
    }
    std::unique_lock<std::mutex> local_lock(local_mutex);
    ensure_flock_loaded();
    hako_flock_acquire(flock_handle);
    MasterLockInspection inspection = inspect_locked(flock_handle);
    if (inspection.health != MasterLockHealth::Ok ||
        inspection.state != MasterLockState::Locked ||
        inspection.owner_pid != owner.pid ||
        inspection.owner_start_token != owner.start_token) {
        hako_flock_release(flock_handle);
        if (inspection.health == MasterLockHealth::Ok) {
            inspection.health = MasterLockHealth::Unknown;
            inspection.reason = "master unlock attempted by a non-owner process";
        }
        throw_lock_health(inspection);
    }
    MasterLockMetadataV1 metadata{};
    if (!read_metadata(flock_handle, &metadata)) {
        hako_flock_release(flock_handle);
        inspection.health = MasterLockHealth::Unknown;
        inspection.reason = "master lock metadata disappeared during release";
        throw_lock_health(inspection);
    }
    metadata.state = static_cast<uint32_t>(MasterLockState::Releasing);
    if (!write_metadata(flock_handle, &metadata)) {
        hako_flock_release(flock_handle);
        inspection.health = MasterLockHealth::Abort;
        inspection.reason = "failed to record master lock release";
        throw_lock_health(inspection);
    }
    const int unlocked_counter = 1;
    if (!hako_flock_write_bytes(
            flock_handle, 0, &unlocked_counter, sizeof(unlocked_counter))) {
        hako_flock_release(flock_handle);
        inspection.health = MasterLockHealth::Abort;
        inspection.reason = "master lock counter release failed";
        throw_lock_health(inspection);
    }
    metadata.state = static_cast<uint32_t>(MasterLockState::Unlocked);
    metadata.owner_pid = 0;
    metadata.owner_start_token = 0;
    if (!write_metadata(flock_handle, &metadata)) {
        hako_flock_release(flock_handle);
        inspection.health = MasterLockHealth::Abort;
        inspection.reason = "master lock release metadata was not committed";
        throw_lock_health(inspection);
    }
    hako_flock_release(flock_handle);
}
