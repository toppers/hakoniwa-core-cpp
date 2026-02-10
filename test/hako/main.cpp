#include <stdio.h>
#include <gtest/gtest.h>

#include "hako.hpp"
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include <unistd.h>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>


int main(int argc, char *argv[])
{
    // Ensure tests use mmap with a writable path.
    const std::string base_dir = "/tmp/hakoniwa";
    const std::string mmap_dir = "/tmp/hakoniwa/mmap";
    (void)mkdir(base_dir.c_str(), 0755);
    (void)mkdir(mmap_dir.c_str(), 0755);

    const std::string config_path = "/tmp/hako_test_config.json";
    std::ofstream ofs(config_path);
    ofs << "{\n"
           "    \"shm_type\": \"mmap\",\n"
           "    \"core_mmap_path\": \"" << mmap_dir << "\"\n"
           "}\n";
    ofs.close();
    setenv("HAKO_CONFIG_PATH", config_path.c_str(), 1);

    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();
    return result;
}
