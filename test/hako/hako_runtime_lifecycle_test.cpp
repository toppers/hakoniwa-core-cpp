#include <gtest/gtest.h>

#include "utils/hako_share/hako_runtime_lifecycle.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace {

class LifecycleDirectory {
public:
    LifecycleDirectory()
    {
        const auto unique = std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        path_ = std::filesystem::temp_directory_path() /
                ("hako-lifecycle-" + unique);
        std::filesystem::create_directories(path_);
    }
    ~LifecycleDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    const std::filesystem::path& path() const { return path_; }
private:
    std::filesystem::path path_;
};

} // namespace

TEST(HakoRuntimeLifecycleTest, LiveParticipantMakesRecoveryBusy)
{
    using namespace hako::utils::lifecycle;
    LifecycleDirectory directory;
    {
        RuntimeParticipantLease participant;
        participant.join(directory.path().string(), true);
        ASSERT_TRUE(participant.joined());

        RuntimeRecoveryGuard recovery;
        std::string reason;
        EXPECT_EQ(recovery.acquire(directory.path().string(), &reason),
                  RuntimeLifecycleHealth::Busy);
        EXPECT_FALSE(recovery.acquired());
    }

    RuntimeRecoveryGuard recovery;
    std::string reason;
    EXPECT_EQ(recovery.acquire(directory.path().string(), &reason),
              RuntimeLifecycleHealth::Ok) << reason;
    EXPECT_TRUE(recovery.acquired());
    EXPECT_EQ(recovery.inactive_leases().size(), 1U);
}

TEST(HakoRuntimeLifecycleTest, LegacyArtifactsAreNotSilentlyUpgraded)
{
    using namespace hako::utils::lifecycle;
    LifecycleDirectory directory;
    std::ofstream(directory.path() / "flock.bin", std::ios::binary)
        << "legacy";

    RuntimeParticipantLease participant;
    try {
        participant.join(directory.path().string(), true);
        FAIL() << "legacy runtime unexpectedly joined";
    }
    catch (const RuntimeLifecycleError& error) {
        EXPECT_EQ(error.health(), RuntimeLifecycleHealth::Unknown);
        EXPECT_NE(std::string(error.what()).find("legacy"), std::string::npos);
    }
    EXPECT_FALSE(participant.joined());
    EXPECT_EQ(std::filesystem::file_size(directory.path() / "flock.bin"), 6U);
}

TEST(HakoRuntimeLifecycleTest, CorruptMarkerIsUnknown)
{
    using namespace hako::utils::lifecycle;
    LifecycleDirectory directory;
    {
        RuntimeParticipantLease participant;
        participant.join(directory.path().string(), true);
    }
    std::ofstream(directory.path() / "runtime-lifecycle-v1.bin",
                  std::ios::binary | std::ios::trunc) << "broken";

    RuntimeRecoveryGuard recovery;
    std::string reason;
    EXPECT_EQ(recovery.acquire(directory.path().string(), &reason),
              RuntimeLifecycleHealth::Unknown);
    EXPECT_FALSE(recovery.acquired());
}

TEST(HakoRuntimeLifecycleTest, MixedGenerationIsUnknown)
{
    using namespace hako::utils::lifecycle;
    LifecycleDirectory directory;
    uint64_t generation = 0;
    {
        RuntimeParticipantLease participant;
        participant.join(directory.path().string(), true);
        generation = participant.generation();
    }
    hako::utils::SharedMemoryMetaDataType metadata{};
    metadata.magic = HAKO_SHM_MAGIC;
    metadata.version = HAKO_SHM_LAYOUT_VERSION;
    metadata.data_size = 1;
    metadata.lifecycle_generation = generation + 1;
    std::ofstream stream(directory.path() / "mmap-0xff.bin", std::ios::binary);
    stream.write(reinterpret_cast<const char*>(&metadata), sizeof(metadata));
    stream.put('\0');
    stream.close();

    RuntimeRecoveryGuard recovery;
    std::string reason;
    EXPECT_EQ(recovery.acquire(directory.path().string(), &reason),
              RuntimeLifecycleHealth::Unknown);
    EXPECT_NE(reason.find("generation"), std::string::npos);
}
