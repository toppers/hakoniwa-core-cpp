#ifndef _HAKO_CONFIG_LOADER_HPP_
#define _HAKO_CONFIG_LOADER_HPP_

#include "nlohmann/json.hpp"
#include "types/hako_types.hpp"
#include <cstdlib> // for std::getenv
#include <fstream>
#include <iostream>

typedef struct {
    nlohmann::json param;
} HakoConfigType;

static inline void hako_config_load(HakoConfigType &config)
{
    const char* env_path = std::getenv("HAKO_CONFIG_PATH");
    std::string config_path = (env_path != nullptr) ? env_path : HAKO_CONFIG_DEFAULT_PATH;
    
    std::ifstream ifs(config_path);
    if (!ifs) {
        config.param = nullptr;
    }
    else {
        config.param = nlohmann::json::parse(ifs);
    }
}


#endif /* _HAKO_CONFIG_LOADER_HPP_ */