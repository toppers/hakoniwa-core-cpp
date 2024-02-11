#include <stdio.h>
#include <gtest/gtest.h>

#include "hako.hpp"
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include <unistd.h>


int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();
    return result;
}
