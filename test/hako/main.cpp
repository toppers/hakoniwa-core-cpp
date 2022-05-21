#include <stdio.h>
#include <gtest/gtest.h>

#include "hako.hpp"
#include "utils/hako_shared_memory.hpp"
#include <unistd.h>

void do_shared_memory_multi_proc_test()
{
    std::shared_ptr<hako::utils::HakoSharedMemory> shm = std::make_shared<hako::utils::HakoSharedMemory>();
    int32_t shmid = shm->create_memory(HAKO_SHARED_MEMORY_ID_0, 1024);
    EXPECT_TRUE(shmid > 0);

    void *value = shm->lock_memory(HAKO_SHARED_MEMORY_ID_0);
    EXPECT_TRUE(value != nullptr);

    for (int i = 0; i < 5; i++)
    {
        printf("LOCKING:sleep 1sec...\n");
        usleep (1000000);
    }

    shm->unlock_memory(HAKO_SHARED_MEMORY_ID_0);

    for (int i = 0; i < 5; i++)
    {
        printf("UNLOCKING:sleep 1sec...\n");
        usleep (1000000);
    }

    shm->destroy_memory(HAKO_SHARED_MEMORY_ID_0);
    return;
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();
    return result;
}
