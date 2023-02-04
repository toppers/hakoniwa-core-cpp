#ifndef _HAKO_LOGGER_HPP_
#define _HAKO_LOGGER_HPP_

#include "types/hako_types.hpp"
//#include "spdlog/spdlog.h"

namespace hako::utils::logger {
    void init(const std::string &id);
    //std::shared_ptr<spdlog::logger> get(const std::string &id);
}

#endif /* _HAKO_LOGGER_HPP_ */
