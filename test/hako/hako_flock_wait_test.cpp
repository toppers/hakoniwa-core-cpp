#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_share/impl/hako_flock_wait.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"
#include "utils/hako_share/hako_master_lock.hpp"
#include "config/hako_config.hpp"

namespace {

class FlockFile {
public:
    FlockFile()
    {
        static std::atomic<unsigned int> sequence{0};
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("hako-flock-wait-" + std::to_string(timestamp) + "-" +
                 std::to_string(++sequence) + ".bin");
        std::string path_string = path_.string();
        first_ = hako_flock_create(path_string);
        second_ = hako_flock_open(path_string);
    }

    ~FlockFile()
    {
        if (second_ != nullptr) {
            hako_flock_destroy(second_);
        }
        if (first_ != nullptr) {
            hako_flock_destroy(first_);
        }
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    HakoFlockObjectType* first() const { return first_; }
    HakoFlockObjectType* second() const { return second_; }

private:
    std::filesystem::path path_;
    HakoFlockObjectType* first_ = nullptr;
    HakoFlockObjectType* second_ = nullptr;
};

class MmapConfigDirectory {
public:
    MmapConfigDirectory()
    {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path() /
                     ("hako-flock-sem-" + std::to_string(timestamp));
        std::filesystem::create_directories(directory_);
        config_path_ = directory_ / "cpp_core_config.json";
        std::ofstream config(config_path_);
        config << "{\n"
               << "  \"shm_type\": \"mmap\",\n"
               << "  \"core_mmap_path\": \"" << directory_.generic_string() << "\"\n"
               << "}\n";
        const char* previous = std::getenv("HAKO_CONFIG_PATH");
        had_previous_ = previous != nullptr;
        if (had_previous_) {
            previous_ = previous;
        }
#ifdef _WIN32
        _putenv_s("HAKO_CONFIG_PATH", config_path_.string().c_str());
#else
        setenv("HAKO_CONFIG_PATH", config_path_.string().c_str(), 1);
#endif
    }

    ~MmapConfigDirectory()
    {
#ifdef _WIN32
        _putenv_s("HAKO_CONFIG_PATH", had_previous_ ? previous_.c_str() : "");
#else
        if (had_previous_) {
            setenv("HAKO_CONFIG_PATH", previous_.c_str(), 1);
        }
        else {
            unsetenv("HAKO_CONFIG_PATH");
        }
#endif
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

private:
    std::filesystem::path directory_;
    std::filesystem::path config_path_;
    bool had_previous_ = false;
    std::string previous_;
};

} // namespace

TEST(HakoFlockWaitTest, ExistingCallersRemainBlockingModeByDefault)
{
    EXPECT_FALSE(hako_flock_wait_is_enabled());
}

TEST(HakoFlockWaitTest, AcquiresImmediatelyWhenFileIsUnlocked)
{
    FlockFile file;
    ASSERT_NE(file.first(), nullptr);

    volatile sig_atomic_t interrupted = 0;
    HakoFlockWaitScope scope({1000, &interrupted});
    EXPECT_NO_THROW(hako_flock_acquire(file.first()));
    EXPECT_EQ(file.first()->lock_count, 1);
    hako_flock_release(file.first());
}

TEST(HakoFlockWaitTest, GrowingLockedFileDoesNotChangeWindowsUnlockRange)
{
    FlockFile file;
    ASSERT_NE(file.first(), nullptr);
    ASSERT_NE(file.second(), nullptr);

    const uint64_t original_size = hako_flock_size(file.first());
    const unsigned char metadata[48]{};
    hako_flock_acquire(file.first());
    ASSERT_TRUE(hako_flock_write_bytes(
        file.first(), original_size, metadata, sizeof(metadata)));
    EXPECT_NO_THROW(hako_flock_release(file.first()));

    volatile sig_atomic_t interrupted = 0;
    HakoFlockWaitScope scope({1000, &interrupted});
    EXPECT_NO_THROW(hako_flock_acquire(file.second()));
    EXPECT_NO_THROW(hako_flock_release(file.second()));
}

TEST(HakoFlockWaitTest, TimesOutWithoutClaimingHeldFileLock)
{
    FlockFile file;
    ASSERT_NE(file.first(), nullptr);
    ASSERT_NE(file.second(), nullptr);
    hako_flock_acquire(file.first());

    volatile sig_atomic_t interrupted = 0;
    const auto started = std::chrono::steady_clock::now();
    {
        HakoFlockWaitScope scope({80, &interrupted});
        try {
            hako_flock_acquire(file.second());
            FAIL() << "Expected a bounded file-lock timeout";
        }
        catch (const HakoFlockWaitError& e) {
            EXPECT_EQ(e.type(), HakoFlockWaitErrorType::Timeout);
        }
    }
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();
    EXPECT_GE(elapsed_ms, 60);
    EXPECT_LT(elapsed_ms, 1000);
    EXPECT_EQ(file.second()->lock_count, 0);
    hako_flock_release(file.first());
}

TEST(HakoFlockWaitTest, InterruptsWithoutClaimingHeldFileLock)
{
    FlockFile file;
    ASSERT_NE(file.first(), nullptr);
    ASSERT_NE(file.second(), nullptr);
    hako_flock_acquire(file.first());

    volatile sig_atomic_t interrupted = SIGTERM;
    HakoFlockWaitScope scope({1000, &interrupted});
    try {
        hako_flock_acquire(file.second());
        FAIL() << "Expected an interrupted file-lock wait";
    }
    catch (const HakoFlockWaitError& e) {
        EXPECT_EQ(e.type(), HakoFlockWaitErrorType::Interrupted);
    }
    EXPECT_EQ(file.second()->lock_count, 0);
    hako_flock_release(file.first());
}

TEST(HakoFlockWaitTest, TimesOutWhileLogicalMasterLockIsUnavailable)
{
    MmapConfigDirectory config;
    constexpr int32_t sem_id = 0x00FF;
    ASSERT_GE(hako::utils::sem::flock::create(sem_id), 0);
    hako::utils::sem::flock::master_lock(sem_id);

    volatile sig_atomic_t interrupted = 0;
    {
        HakoFlockWaitScope scope({80, &interrupted});
        try {
            hako::utils::sem::flock::master_lock(sem_id);
            FAIL() << "Expected a bounded logical master-lock timeout";
        }
        catch (const HakoFlockWaitError& e) {
            EXPECT_EQ(e.type(), HakoFlockWaitErrorType::Timeout);
        }
    }

    hako::utils::sem::flock::master_unlock(sem_id);
    {
        HakoFlockWaitScope scope({1000, &interrupted});
        EXPECT_NO_THROW(hako::utils::sem::flock::master_lock(sem_id));
        hako::utils::sem::flock::master_unlock(sem_id);
    }
    hako::utils::sem::flock::destroy(sem_id);
}

TEST(HakoFlockWaitTest, MasterLockMetadataTracksNormalAcquireAndRelease)
{
    using namespace hako::utils::sem::flock;
    MmapConfigDirectory config;
    constexpr int32_t sem_id = 0x00FF;
    ASSERT_GE(create(sem_id), 0);
    auto value = inspect_master_lock(100, false);
    EXPECT_EQ(value.health, MasterLockHealth::Ok);
    EXPECT_EQ(value.state, MasterLockState::Unlocked);

    master_lock(sem_id);
    value = inspect_master_lock(100, false);
    EXPECT_EQ(value.health, MasterLockHealth::Ok);
    EXPECT_EQ(value.state, MasterLockState::Locked);
    EXPECT_GT(value.owner_pid, 0);

    master_unlock(sem_id);
    value = inspect_master_lock(100, false);
    EXPECT_EQ(value.health, MasterLockHealth::Ok);
    EXPECT_EQ(value.state, MasterLockState::Unlocked);
    EXPECT_EQ(value.owner_pid, 0);
    destroy(sem_id);
}

TEST(HakoFlockWaitTest, CleanMasterStartResetsStaleAssetSemaphore)
{
    using namespace hako::utils::sem::flock;
    MmapConfigDirectory config;
    constexpr int32_t sem_id = HAKO_SHARED_MEMORY_ID_0;
    ASSERT_GE(create(sem_id), 0);
    asset_up(sem_id, 0);
    ASSERT_GE(create(sem_id), 0);
    volatile sig_atomic_t interrupted = 0;
    HakoFlockWaitScope scope({80, &interrupted});
    EXPECT_THROW(asset_down(sem_id, 0), HakoFlockWaitError);
    destroy(sem_id);
}

TEST(HakoFlockWaitTest, SecondaryMemoryCreateDoesNotResetAssetSemaphore)
{
    using namespace hako::utils::sem::flock;
    MmapConfigDirectory config;
    ASSERT_GE(create(HAKO_SHARED_MEMORY_ID_0), 0);
    asset_up(HAKO_SHARED_MEMORY_ID_0, 0);
    ASSERT_GE(create(HAKO_SHARED_MEMORY_ID_1), 0);
    volatile sig_atomic_t interrupted = 0;
    HakoFlockWaitScope scope({100, &interrupted});
    EXPECT_NO_THROW(asset_down(HAKO_SHARED_MEMORY_ID_1, 0));
    destroy(HAKO_SHARED_MEMORY_ID_1);
}

TEST(HakoFlockWaitTest, LockedMasterDoesNotResetAssetSemaphore)
{
    using namespace hako::utils::sem::flock;
    MmapConfigDirectory config;
    constexpr int32_t sem_id = HAKO_SHARED_MEMORY_ID_0;
    ASSERT_GE(create(sem_id), 0);
    asset_up(sem_id, 0);
    master_lock(sem_id);
    ASSERT_GE(create(sem_id), 0);
    master_unlock(sem_id);
    volatile sig_atomic_t interrupted = 0;
    HakoFlockWaitScope scope({100, &interrupted});
    EXPECT_NO_THROW(asset_down(sem_id, 0));
    destroy(sem_id);
}
