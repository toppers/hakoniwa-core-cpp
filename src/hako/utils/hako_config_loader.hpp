#ifndef _HAKO_CONFIG_LOADER_HPP_
#define _HAKO_CONFIG_LOADER_HPP_

#include "nlohmann/json.hpp"
#include "types/hako_types.hpp"
#include <fstream>
#include <iostream>

typedef struct {
    nlohmann::json param;
} HakoConfigType;

static inline void hako_config_load(HakoConfigType &config)
{
    std::ifstream ifs(HAKO_CONFIG_DEFAULT_PATH);
    if (!ifs) {
        config.param = nullptr;
    }
    else {
        config.param = nlohmann::json::parse(ifs);
    }
}

#endif /* _HAKO_CONFIG_LOADER_HPP_ */