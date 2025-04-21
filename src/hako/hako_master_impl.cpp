#include "hako_master_impl.hpp"
#include "data/hako_master_data.hpp"
#include "hako_simevent_impl.hpp"

bool hako::HakoMasterControllerImpl::execute()
{
    if (this->theWorld_ == nullptr) {
        return false;
    }
    HakoSimulationStateType prev;
    HakoSimulationStateType next;
    bool is_changed = this->sim_event_->do_event_handling(prev, next);
    if (is_changed) {
        if ((prev == HakoSim_Runnable) && (next == HakoSim_Running)) {
#ifdef FIX_PDU_CREATE_TIMING
            // nothing to do
#else
            this->master_data_->create_pdu_data();
#endif
        }
        else if ((prev == HakoSim_Resetting) && (next == HakoSim_Stopped)) {
            this->master_data_->lock();
            auto& timeset = this->master_data_->ref_time_nolock();
            timeset.current = 0;
            this->theWorld_->reset_world_time();
            this->master_data_->unlock();
            this->master_data_->destroy_pdu_data();
        }
    }

    if (next != HakoSim_Running) {
        return false;
    }
    else if (this->master_data_->get_pdu_data()->is_simulation_mode() == false) {
        return false;
    }
    auto prev_world_time = this->theWorld_->get_world_time_usec();

    std::vector<HakoTimeType> asset_times;
    this->master_data_->get_asset_times(asset_times);
    auto world_time = this->theWorld_->time_begins_to_move(asset_times);
    if (world_time > prev_world_time) {
        auto& timeset = this->master_data_->ref_time_nolock();
        timeset.current = world_time;
        return true;
    }
    else {
        //hako::utils::logger::get("core")->info("skip: world_time={0}", world_time);
        return false;
    }
}

void hako::HakoMasterControllerImpl::set_config_simtime(HakoTimeType max_delay_time_usec, HakoTimeType delta_time_usec)
{
    this->theWorld_ = std::make_shared<hako::core::simulation::time::TheWorld>(max_delay_time_usec, delta_time_usec);
    this->master_data_->lock();
    {
        auto & timeset = this->master_data_->ref_time_nolock();
        timeset.max_delay = max_delay_time_usec;
        timeset.delta = delta_time_usec;
    }
    this->master_data_->unlock();
    return;
}

HakoTimeType hako::HakoMasterControllerImpl::get_max_deltay_time_usec()
{
    return this->theWorld_->get_max_delay_time_usec();
}
HakoTimeType hako::HakoMasterControllerImpl::get_delta_time_usec()
{
    return this->theWorld_->get_delta_time_usec();
}
