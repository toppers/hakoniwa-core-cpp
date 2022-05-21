#ifndef _HAKO_SIMEVENT_IMPL_HPP_
#define _HAKO_SIMEVENT_IMPL_HPP_

#include "hako.hpp"
#include "hako_simevent.hpp"
#include "data/hako_master_data.hpp"

namespace hako {
    class HakoSimulationEventController : public IHakoSimulationEventController {
    public:
        HakoSimulationEventController(std::shared_ptr<data::HakoMasterData> ptr)
        {
            this->master_data_ = ptr;
            this->asset_controller_ = hako::create_asset_controller();
        }
        HakoSimulationStateType state();

        /*
         * simulation execution event
         */
        bool start();
        bool stop();
        bool reset();


        /*
         * event monitor
         */
        void do_event_handling();
        bool do_event_handling(HakoSimulationStateType &prev, HakoSimulationStateType &next);
    private:
        void do_event_handling_nolock(std::vector<HakoAssetIdType> *error_assets);
        void do_event_handling_timeout_nolock(std::vector<HakoAssetIdType> *error_assets);
        void publish_event_nolock(hako::data::HakoAssetEventType event);
        bool trigger_event(HakoSimulationStateType curr_state, HakoSimulationStateType next_state, hako::data::HakoAssetEventType event);
        HakoSimulationEventController() {}
        std::shared_ptr<data::HakoMasterData> master_data_;
        std::shared_ptr<hako::IHakoAssetController> asset_controller_;
    };    
}

#endif /* _HAKO_SIMEVENT_IMPL_HPP_ */