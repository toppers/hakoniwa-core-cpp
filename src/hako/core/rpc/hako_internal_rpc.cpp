#include "core/rpc/hako_internal_rpc.hpp"
#include "core/context/hako_context.hpp"

void hako::core::rpc::HakoInternalRpc::register_callback(hako::data::HakoAssetEventType event_id, void (*callback) ())
{
    this->map_.insert(std::make_pair(event_id, callback));
}


void hako::core::rpc::HakoInternalRpc::start()
{
    hako::core::context::HakoContext context;
    if (context.is_same(this->master_data_->get_master_pid())) {
        // nothing to do
    }
    else {
        this->proxy_thread_ = std::make_shared<std::thread>(&hako::core::rpc::HakoInternalRpc::proxy_thread, this);
        this->proxy_thread_->detach();
    }
}

void hako::core::rpc::HakoInternalRpc::stop()
{
    if (this->proxy_thread_ == nullptr) {
        return;
    }
    hako::core::context::HakoContext context;
    if (context.is_same(this->master_data_->get_master_pid())) {
        // nothing to do
    }
    else {
        hako::utils::sem::asset_up(this->master_data_->get_semid(), this->asset_id_);
        this->proxy_thread_->join();
        this->proxy_thread_ = nullptr;
    }
}
void hako::core::rpc::HakoInternalRpc::proxy_thread()
{
    hako::utils::logger::get("core")->info("HakoInternalRpc: monitor_thread start: asset[{0}]", this->asset_id_);
    while (true) {
        hako::utils::sem::asset_down(this->master_data_->get_semid(), this->asset_id_);
        auto* asset = this->master_data_->get_asset_event_nolock(this->asset_id_);
        if (asset == nullptr) {
            break;
        }
        hako::data::HakoAssetEventType event_id = asset->event;
        this->map_[event_id]();
    }
    hako::utils::logger::get("core")->info("HakoInternalRpc: monitor_thread stop: asset[{0}]", this->asset_id_);
    return;
}

void hako::core::rpc::notify(std::shared_ptr<data::HakoMasterData> master_data, HakoAssetIdType asset_id, hako::data::HakoAssetEventType event_id)
{
    hako::core::context::HakoContext context;

    auto* asset_ev = master_data->get_asset_event_nolock(asset_id);
    asset_ev->event = event_id;
    asset_ev->event_feedback = false;
    if (!context.is_same(asset_ev->pid)) {
        hako::utils::sem::asset_up(master_data->get_semid(), asset_id);
        return;
    }
    auto* asset = master_data->get_asset(asset_id);
    switch (event_id) {
        case hako::data::HakoAssetEvent_Start:
            asset->callback.start();
            break;
        case hako::data::HakoAssetEvent_Stop:
            asset->callback.stop();
            break;
        case hako::data::HakoAssetEvent_Reset:
            asset->callback.reset();
            break;
        default:
            break;
    }

    return;
}
