#include "core/rpc/hako_internal_rpc.hpp"
#include "core/context/hako_context.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"
#include "utils/hako_share/hako_sem.hpp"
#include "utils/hako_config_loader.hpp"

typedef struct {
    void (*asset_up)(int32_t sem_id, int32_t asset_id);
    void (*asset_down)(int32_t sem_id, int32_t asset_id);
} SemType;
static SemType my_sem;

void hako::core::rpc::HakoInternalRpc::register_callback(hako::data::HakoAssetEventType event_id, void (*callback) ())
{
    HakoConfigType config;
    hako_config_load(config);
    if ((config.param == nullptr) || (config.param["shm_type"] == "shm")) {
        my_sem.asset_up = utils::sem::asset_up;
        my_sem.asset_down = utils::sem::asset_down;
    }
    else {
        my_sem.asset_up = utils::sem::flock::asset_up;
        my_sem.asset_down = utils::sem::flock::asset_down;
    }
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
        my_sem.asset_up(this->master_data_->get_semid(), this->asset_id_);
        this->proxy_thread_->join();
        this->proxy_thread_ = nullptr;
    }
}
void hako::core::rpc::HakoInternalRpc::proxy_thread()
{
    HAKO_LOG_INFO("HakoInternalRpc Thread: Start");
    while (true) {
        my_sem.asset_down(this->master_data_->get_semid(), this->asset_id_);
        auto* asset = this->master_data_->get_asset_event_nolock(this->asset_id_);
        if (asset == nullptr) {
            break;
        }
        hako::data::HakoAssetEventType event_id = asset->event;
        this->map_[event_id]();
    }
    HAKO_LOG_INFO("HakoInternalRpc Thread: End");
    return;
}

void hako::core::rpc::notify(std::shared_ptr<data::HakoMasterData> master_data, HakoAssetIdType asset_id, hako::data::HakoAssetEventType event_id)
{
    hako::core::context::HakoContext context;

    auto* asset_ev = master_data->get_asset_event_nolock(asset_id);
    asset_ev->event = event_id;
    asset_ev->event_feedback = false;
    auto *asset = master_data->get_asset(asset_id);
    if (asset->callback.start == nullptr) { //polling
        return;
    }
    if (my_sem.asset_up == nullptr) {
        HakoConfigType config;
        hako_config_load(config);
        if ((config.param == nullptr) || (config.param["shm_type"] == "shm")) {
            my_sem.asset_up = utils::sem::asset_up;
            my_sem.asset_down = utils::sem::asset_down;
        }
        else {
            my_sem.asset_up = utils::sem::flock::asset_up;
            my_sem.asset_down = utils::sem::flock::asset_down;
        }
    }

    if (!context.is_same(asset_ev->pid)) {
        my_sem.asset_up(master_data->get_semid(), asset_id);
        return;
    }
    switch (event_id) {
        case hako::data::HakoAssetEvent_Start:
            HAKO_LOG_INFO("Notify Event: Start asset=%s", asset->name);
            asset->callback.start();
            break;
        case hako::data::HakoAssetEvent_Stop:
            HAKO_LOG_INFO("Notify Event: Stop asset=%s", asset->name);
            asset->callback.stop();
            break;
        case hako::data::HakoAssetEvent_Reset:
            HAKO_LOG_INFO("Notify Event: Reset asset=%s", asset->name);
            asset->callback.reset();
            break;
        default:
            break;
    }

    return;
}
