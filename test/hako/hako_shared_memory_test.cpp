#include <gtest/gtest.h>
#include "utils/hako_shared_memory.hpp"

class HakoSharedMemoryTest : public ::testing::Test {
protected:
    static void SetUpTestCase()
    {
    }
    static void TearDownTestCase()
    {
    }
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }

};

TEST_F(HakoSharedMemoryTest, HakoSharedMemory_01)
{
    std::shared_ptr<hako::utils::HakoSharedMemory> shm = std::make_shared<hako::utils::HakoSharedMemory>();
    EXPECT_TRUE(shm.get() != nullptr);

    int32_t shmid = shm->create_memory(HAKO_SHARED_MEMORY_ID_0, 1024);
    EXPECT_TRUE(shmid > 0);

    void *value = shm->lock_memory(HAKO_SHARED_MEMORY_ID_0);
    EXPECT_TRUE(value != nullptr);

    shm->unlock_memory(HAKO_SHARED_MEMORY_ID_0);

    shm->destroy_memory(HAKO_SHARED_MEMORY_ID_0);

}
