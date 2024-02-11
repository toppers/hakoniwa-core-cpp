#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include "utils/hako_share/hako_shared_memory_shm.hpp"
#include "utils/hako_share/hako_shared_memory_mmap.hpp"

std::shared_ptr<hako::utils::HakoSharedMemory> hako::utils::hako_shared_memory_create(const std::string& type)
{
    if (type == "shm") {
        return std::make_shared<hako::utils::HakoSharedMemoryShm>();
    }
    else if (type == "mmap") {
        return std::make_shared<hako::utils::HakoSharedMemoryMmap>();
    }
    else
    {
        return nullptr;
    }
}