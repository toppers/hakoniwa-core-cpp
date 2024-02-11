#include "utils/hako_share/hako_shared_memory_factory.hpp"
#include "utils/hako_share/hako_shared_memory_shm.hpp"

std::shared_ptr<hako::utils::HakoSharedMemory> hako::utils::hako_shared_memory_create(const std::string& type)
{
    if (type == "shm") {
        return std::make_shared<hako::utils::HakoSharedMemoryShm>();
    }
    else
    {
        return nullptr;
    }
}