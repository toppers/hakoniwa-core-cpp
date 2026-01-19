#include "hako_simevent_impl.hpp"
#include "hako_log.hpp"
#include "core/rpc/hako_internal_rpc.hpp"

HakoSimulationStateType hako::HakoSimulationEventController::state()
{
    auto& state = this->master_data_->ref_state_nolock();
    return state;
}

bool hako::HakoSimulationEventController::trigger_event(HakoSimulationStateType curr_state, HakoSimulationStateType next_state, hako::data::HakoAssetEventType event)
{
    bool ret = true;
    this->master_data_->lock();
    {
        auto& state = this->master_data_->ref_state_nolock();
        if (state == curr_state) {
            state = next_state;
            for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
                auto* asset_ev = this->master_data_->get_asset_event_nolock(i);
                if (asset_ev != nullptr) {
                    asset_ev->event_feedback = false;
                }
            }
        }
        else if (next_state == HakoSim_Error) {
            state = next_state;
        }
        else {
            ret = false;
        }
    }
    this->master_data_->unlock();

    if (ret) {
        this->publish_event_nolock(event);
    }
    return ret;
}

bool hako::HakoSimulationEventController::start()
{
    HAKO_LOG_INFO("Event: Start");
    return this->trigger_event(HakoSim_Stopped, HakoSim_Runnable, hako::data::HakoAssetEvent_Start);
}



bool hako::HakoSimulationEventController::stop()
{
    HAKO_LOG_INFO("Event: Stop");
    return this->trigger_event(HakoSim_Running, HakoSim_Stopping, hako::data::HakoAssetEvent_Stop);
}

bool hako::HakoSimulationEventController::reset()
{
    HAKO_LOG_INFO("Event: Reset");
    bool ret = false;
    auto& state = this->master_data_->ref_state_nolock();
    if (state == HakoSim_Stopped) {
        ret = this->trigger_event(HakoSim_Stopped, HakoSim_Resetting, hako::data::HakoAssetEvent_Reset);
    }
    else if (state == HakoSim_Error) {
        ret = this->trigger_event(HakoSim_Error, HakoSim_Stopped, hako::data::HakoAssetEvent_Reset);
    }
    return ret;
}
bool hako::HakoSimulationEventController::assets(std::vector<std::shared_ptr<std::string>> & asset_list) 
{
    this->master_data_->get_asset_names(asset_list);
    return true;
}
HakoPduChannelIdType hako::HakoSimulationEventController::get_pdu_channel(const std::string& asset_name, HakoPduChannelIdType channel_id)
{
    return this->master_data_->get_pdu_data()->get_pdu_channel(asset_name, channel_id);
}
bool hako::HakoSimulationEventController::write_pdu(HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    return this->master_data_->get_pdu_data()->write_pdu_nolock(channel_id, pdu_data, len);
}
bool hako::HakoSimulationEventController::read_pdu(HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    return this->master_data_->get_pdu_data()->read_pdu_nolock(channel_id, pdu_data, len);
}
size_t hako::HakoSimulationEventController::pdu_size(HakoPduChannelIdType channel_id)
{
    return this->master_data_->get_pdu_data()->pdu_size_nolock(channel_id);
}

void hako::HakoSimulationEventController::print_master_data()
{
    this->master_data_->print_master_data();
}
void hako::HakoSimulationEventController::print_memory_log()
{
    this->master_data_->print_log();
}

void hako::HakoSimulationEventController::do_event_handling()
{
    HakoSimulationStateType prev;
    HakoSimulationStateType next;
    (void)this->do_event_handling(prev, next);
}
bool hako::HakoSimulationEventController::do_event_handling(HakoSimulationStateType &prev, HakoSimulationStateType &next)
{
    bool is_changed = false;
    std::vector<HakoAssetIdType> error_assets;
    this->master_data_->lock();
    prev = this->master_data_->ref_state_nolock();
    this->do_event_handling_nolock(&error_assets);
    next = this->master_data_->ref_state_nolock();
    is_changed = (prev != next);
    this->master_data_->unlock();

    if (error_assets.size() > 0) {
        (void)this->trigger_event(HakoSim_Any, HakoSim_Error, hako::data::HakoAssetEvent_Error);
    }

    for (auto& asset_id : error_assets) {
        hako::data::HakoAssetEntryType *entry = this->master_data_->get_asset(asset_id);
        std::shared_ptr<std::string>  asset_name = hako::utils::hako_fixed2string(entry->name);
        //hako::utils::logger::get("core")->error("asset[{0}] timeout", *asset_name);
        this->asset_controller_->asset_unregister(*asset_name);
    }
    return is_changed;
}

void hako::HakoSimulationEventController::do_event_handling_nolock(std::vector<HakoAssetIdType> *error_assets)
{
    auto& state = this->master_data_->ref_state_nolock();
    switch (state) {
        case HakoSim_Runnable:
            if (this->master_data_->is_all_feedback_nolock()) {
                state = HakoSim_Running;
            }
            else if (error_assets != nullptr) {
                this->do_event_handling_timeout_nolock(error_assets);
            }
            break;
        case HakoSim_Stopping:
        case HakoSim_Resetting:
            if (this->master_data_->is_all_feedback_nolock()) {
                state = HakoSim_Stopped;
            }
            else if (error_assets != nullptr) {
                this->do_event_handling_timeout_nolock(error_assets);
            }
            break;
        case HakoSim_Running:
            /* nothing to do */
            break;
        case HakoSim_Stopped:
        case HakoSim_Terminated:
        default:
            break;
    }

    return;
}
void hako::HakoSimulationEventController::do_event_handling_timeout_nolock(std::vector<HakoAssetIdType> *error_assets)
{
    if (error_assets == nullptr) {
        return;
    }
    for (int id = 0; id < HAKO_DATA_MAX_ASSET_NUM; id++)
    {
        auto* asset_ev = this->master_data_->get_asset_event_nolock(id);
        if (asset_ev != nullptr) {
            if (this->master_data_->is_asset_timeout_nolock(id)) {
                HakoSimulationStateType state = this->master_data_->ref_state_nolock();
                //hako::utils::logger::get("core")->error("TIMEOUT:asset_id={0} state={1}", id, state);
                std::cout << "ERROR: timeout asset_id=" << id << "stated=" << state << std::endl;
                error_assets->push_back(id);
            }
        }
    }
}


void hako::HakoSimulationEventController::publish_event_nolock(hako::data::HakoAssetEventType event)
{
#ifdef FIX_PDU_CREATE_TIMING
    if (event == hako::data::HakoAssetEvent_Start) {
        this->master_data_->create_pdu_data();
    }
#endif
    for (int i = 0; i < HAKO_DATA_MAX_ASSET_NUM; i++) {
        hako::data::HakoAssetEntryType* entry = this->master_data_->get_asset(i);
        if (entry == nullptr) {
            continue;
        }
        hako::data::HakoAssetEntryEventType* entry_ev = this->master_data_->get_asset_event_nolock(i);
        if (entry->type != hako::data::HakoAssetType::HakoAsset_Unknown) {
            switch (event) {
                case hako::data::HakoAssetEvent_Reset:
                    if (hako::data::HakoAssetEvent_Error == entry_ev->event) {
                        entry_ev->event_feedback = true;
                    }
                    else {
                        hako::core::rpc::notify(this->master_data_, i, event);
                    }
                    break;
                case hako::data::HakoAssetEvent_Start:
                case hako::data::HakoAssetEvent_Stop:
                    hako::core::rpc::notify(this->master_data_, i, event);
                    break;
                case hako::data::HakoAssetEvent_Error:
                    entry_ev->event = event;
                    entry_ev->event_feedback = true;
                    break;
                default:
                    break;
            }
        }
    }
}
