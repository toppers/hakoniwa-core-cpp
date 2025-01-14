#include "hako_asset_impl.hpp"
//#include "utils/hako_logger.hpp"
#include "utils/hako_clock.hpp"

bool hako::HakoAssetControllerImpl::asset_register(const std::string & name, AssetCallbackType &callbacks)
{
    hako::core::context::HakoContext context;
    auto id = this->master_data_->alloc_asset(name, hako::data::HakoAssetType::HakoAsset_Inside, &callbacks);
    if (id < 0) {
        HAKO_LOG_ERROR("can not registered: asset: %s", name.c_str());
        return false;
    }
    else {
        HAKO_LOG_INFO("registered: asset: %s", name.c_str());
    }
    this->rpc_ = std::make_shared<hako::core::rpc::HakoInternalRpc>(id, this->master_data_);
    this->rpc_->register_callback(hako::data::HakoAssetEvent_Start, callbacks.start);
    this->rpc_->register_callback(hako::data::HakoAssetEvent_Stop, callbacks.stop);
    this->rpc_->register_callback(hako::data::HakoAssetEvent_Reset, callbacks.reset);
    this->rpc_->start();
    return true;
}
bool hako::HakoAssetControllerImpl::asset_register_polling(const std::string & name)
{
    hako::core::context::HakoContext context;
    auto id = this->master_data_->alloc_asset(name, hako::data::HakoAssetType::HakoAsset_Inside, nullptr);
    if (id < 0) {
        HAKO_LOG_ERROR("can not registered: asset: %s", name.c_str());
        return false;
    }
    else {
        HAKO_LOG_INFO("registered: asset: %s", name.c_str());
    }
    return true;
}
HakoSimulationAssetEventType hako::HakoAssetControllerImpl::asset_get_event(const std::string & name)
{
    auto* asset = this->master_data_->get_asset_nolock(name);
    if (asset == nullptr) {
        return HakoSimulationAssetEventType::HakoSimAssetEvent_Error;
    }
    auto* asset_event = this->master_data_->get_asset_event_nolock(asset->id);
    if (asset_event == nullptr) {
        return HakoSimulationAssetEventType::HakoSimAssetEvent_Error;
    }
    auto ret = asset_event->event;
    return (HakoSimulationAssetEventType)ret;
}

bool hako::HakoAssetControllerImpl::asset_unregister(const std::string & name)
{
    auto ret = this->master_data_->free_asset(name);
    if (ret) {
        HAKO_LOG_ERROR("can not unregistered: asset: %s", name.c_str());
    }
    else {
        HAKO_LOG_INFO("unregistered: asset: %s", name.c_str());
    }
    if (this->rpc_ != nullptr) {
        this->rpc_->stop();
    }
    return ret;
}
void hako::HakoAssetControllerImpl::notify_simtime(const std::string & name, HakoTimeType simtime)
{
    //this->master_data_->lock();
    auto* asset = this->master_data_->get_asset_nolock(name);
    if (asset != nullptr) {
        auto* asset_event = this->master_data_->get_asset_event_nolock(asset->id);
        asset_event->ctime = simtime;
        //hako::utils::logger::get("core")->info("notify_simtime: {0} time={1}", name, simtime);
        asset_event->update_time = hako_get_clock();
    }
    //this->master_data_->unlock();
    return;
}
HakoTimeType hako::HakoAssetControllerImpl::get_worldtime()
{
    hako::data::HakoTimeSetType timeset = this->master_data_->ref_time_nolock();
    return timeset.current;
}


bool hako::HakoAssetControllerImpl::feedback(const std::string& asset_name, bool isOk, HakoSimulationStateType exp_state)
{
    bool ret = true;
    this->master_data_->lock();
    {
        auto& state = this->master_data_->ref_state_nolock();
        auto* entry = this->master_data_->get_asset_nolock(asset_name);
        if (entry != nullptr) {
            auto* entry_ev = this->master_data_->get_asset_event_nolock(entry->id);
            entry_ev->update_time = hako_get_clock();
            if (state == exp_state) {
                entry_ev->event_feedback = isOk;
            }
            else {
                entry_ev->event_feedback = false;
                ret = false;
            }
            entry_ev->event = hako::data::HakoAssetEventType::HakoAssetEvent_None;
        }
        else {
            ret = false;
        }
        //delete logs because deadlock occured... 
        //hako::utils::logger::get("core")->info("feedback isOk={0} ret={1} state={2} exp_state={3}", isOk, ret, state, exp_state);
        //hako::utils::logger::get("core")->flush();
    }
    this->master_data_->unlock();
    return ret;
}

bool hako::HakoAssetControllerImpl::start_feedback(const std::string& asset_name, bool isOk)
{
    HAKO_LOG_INFO("start feedbak: asset: %s %d", asset_name.c_str(), isOk);
    return this->feedback(asset_name, isOk, HakoSim_Runnable);
}
bool hako::HakoAssetControllerImpl::stop_feedback(const std::string& asset_name, bool isOk)
{
    HAKO_LOG_INFO("stop feedbak: asset: %s %d", asset_name.c_str(), isOk);
    return this->feedback(asset_name, isOk, HakoSim_Stopping);
}
bool hako::HakoAssetControllerImpl::reset_feedback(const std::string& asset_name, bool isOk)
{
    HAKO_LOG_INFO("reset feedbak: asset: %s %d", asset_name.c_str(), isOk);
    return this->feedback(asset_name, isOk, HakoSim_Resetting);
}
bool hako::HakoAssetControllerImpl::create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size)
{
    HAKO_LOG_INFO("create pdu channel: channel_id= %d pdu_size=%ld", channel_id, pdu_size);
    return this->master_data_->get_pdu_data()->create_channel(channel_id, pdu_size);
}
bool hako::HakoAssetControllerImpl::create_pdu_lchannel(const std::string& robo_name, HakoPduChannelIdType channel_id, size_t pdu_size)
{
    HAKO_LOG_INFO("create pdu lchannel: robo_name=%s channel_id= %d pdu_size=%ld", robo_name.c_str(), channel_id, pdu_size);
    return this->master_data_->get_pdu_data()->create_lchannel(robo_name, channel_id, pdu_size);
}
HakoPduChannelIdType hako::HakoAssetControllerImpl::get_pdu_channel(const std::string& robo_name, HakoPduChannelIdType channel_id)
{
    return this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
}
bool hako::HakoAssetControllerImpl::is_pdu_dirty(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id)
{
    auto* asset = this->master_data_->get_asset_nolock(asset_name);
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->is_pdu_dirty(asset->id, real_id);
    }
    else {
        return this->master_data_->get_pdu_data()->is_pdu_dirty(asset->id, channel_id);
    }
}

bool hako::HakoAssetControllerImpl::write_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    auto* asset = this->master_data_->get_asset_nolock(asset_name);
    if (asset == nullptr) {
        //hako::utils::logger::get("core")->info("write_pdu: can not find asset[{0}]", asset_name);
        return false;
    }
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->write_pdu(real_id, pdu_data, len);
    }
    else {
        return this->master_data_->get_pdu_data()->write_pdu(channel_id, pdu_data, len);
    }
}
bool hako::HakoAssetControllerImpl::write_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->write_pdu(real_id, pdu_data, len);
    }
    else {
        return this->master_data_->get_pdu_data()->write_pdu(channel_id, pdu_data, len);
    }
}

bool hako::HakoAssetControllerImpl::read_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    auto* asset = this->master_data_->get_asset_nolock(asset_name);
    if (asset == nullptr) {
        //hako::utils::logger::get("core")->info("read_pdu: can not find asset[{0}]", asset_name);
        return false;
    }
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->read_pdu(asset->id, real_id, pdu_data, len);
    }
    else {
        return this->master_data_->get_pdu_data()->read_pdu(asset->id, channel_id, pdu_data, len);
    }
}
bool hako::HakoAssetControllerImpl::read_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->read_pdu_for_external(real_id, pdu_data, len);
    }
    else {
        return this->master_data_->get_pdu_data()->read_pdu_for_external(channel_id, pdu_data, len);
    }
}
bool hako::HakoAssetControllerImpl::write_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)
{
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->write_pdu_nolock(real_id, pdu_data, len);
    }
    else {
        return false;
    }
}

bool hako::HakoAssetControllerImpl::read_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)
{
    HakoPduChannelIdType real_id = this->master_data_->get_pdu_data()->get_pdu_channel(robo_name, channel_id);
    if (real_id >= 0) {
        return this->master_data_->get_pdu_data()->read_pdu_nolock(real_id, pdu_data, len);
    }
    else {
        return false;
    }
}

void hako::HakoAssetControllerImpl::notify_read_pdu_done(const std::string& asset_name)
{
    auto* asset = this->master_data_->get_asset_nolock(asset_name);
    if (asset == nullptr) {
        return;
    }
    this->master_data_->get_pdu_data()->notify_read_pdu_done(asset->id);
}

void hako::HakoAssetControllerImpl::notify_write_pdu_done(const std::string& asset_name)
{
    auto* asset = this->master_data_->get_asset_nolock(asset_name);
    if (asset == nullptr) {
        return;
    }
    this->master_data_->get_pdu_data()->notify_write_pdu_done(asset->id);
    return;
}

bool hako::HakoAssetControllerImpl::is_pdu_sync_mode(const std::string& asset_name)
{
    auto* asset = this->master_data_->get_asset_nolock(asset_name);
    if (asset == nullptr) {
        return false;
    }
    return this->master_data_->get_pdu_data()->is_pdu_sync_mode(asset->id);
}
bool hako::HakoAssetControllerImpl::is_simulation_mode()
{
    return this->master_data_->get_pdu_data()->is_simulation_mode();
}
bool hako::HakoAssetControllerImpl::is_pdu_created()
{
    return this->master_data_->get_pdu_data()->is_pdu_created();
}
