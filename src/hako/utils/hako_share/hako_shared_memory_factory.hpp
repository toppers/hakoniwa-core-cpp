#ifndef _HAKO_SHARED_MEMORY_FACTORY_HPP_
#define _HAKO_SHARED_MEMORY_FACTORY_HPP_

#include "hako_shared_memory.hpp"

namespace hako::utils {
    std::shared_ptr<hako::utils::HakoSharedMemory> hako_shared_memory_create(const std::string& type);
}

#endif /* _HAKO_SHARED_MEMORY_FACTORY_HPP_ */