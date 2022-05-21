#ifndef _HAKO_HPP_
#define _HAKO_HPP_

#include "hako_master.hpp"
#include "hako_asset.hpp"
#include "hako_simevent.hpp"
#include "spdlog/spdlog.h"

namespace hako {
    void init();
    void destroy();
    std::shared_ptr<hako::IHakoMasterController> create_master();
    std::shared_ptr<hako::IHakoAssetController> create_asset_controller();
    std::shared_ptr<hako::IHakoSimulationEventController> get_simevent_controller();

    namespace logger {
        void init(const std::string &id);
        std::shared_ptr<spdlog::logger> get(const std::string &id);
    }
}

#endif /* _HAKO_HPP_ */