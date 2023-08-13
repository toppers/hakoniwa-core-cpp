#ifndef _HAKO_HPP_
#define _HAKO_HPP_

#include "hako_master.hpp"
#include "hako_asset.hpp"
#include "hako_simevent.hpp"

namespace hako {
    void init();
    void destroy();
    std::shared_ptr<hako::IHakoMasterController> create_master();
    std::shared_ptr<hako::IHakoAssetController> create_asset_controller();
    std::shared_ptr<hako::IHakoSimulationEventController> get_simevent_controller();
}

#include "hako_log.hpp"

#endif /* _HAKO_HPP_ */