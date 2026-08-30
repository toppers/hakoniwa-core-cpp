#include <stdio.h>
#include <gtest/gtest.h>

#include "hako.hpp"
#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <string>


int main(int argc, char *argv[])
{
    // Ensure tests use mmap with a writable path.
    const std::filesystem::path base_dir =
        std::filesystem::temp_directory_path() / "hakoniwa";
    const std::filesystem::path mmap_dir = base_dir / "mmap";
    std::filesystem::create_directories(mmap_dir);

    const std::filesystem::path config_path =
        std::filesystem::temp_directory_path() / "hako_test_config.json";
    std::ofstream ofs(config_path);
    ofs << "{\n"
           "    \"shm_type\": \"mmap\",\n"
           "    \"core_mmap_path\": \"" << mmap_dir.generic_string() << "\"\n"
           "}\n";
    ofs.close();
#ifdef _WIN32
    _putenv_s("HAKO_CONFIG_PATH", config_path.string().c_str());
#else
    setenv("HAKO_CONFIG_PATH", config_path.string().c_str(), 1);
#endif

    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();
    return result;
}
