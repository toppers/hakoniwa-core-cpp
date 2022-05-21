#ifndef _HAKO_INTERNAL_RPC_HPP_
#define _HAKO_INTERNAL_RPC_HPP_

#include "types/hako_types.hpp"
#include "data/hako_master_data.hpp"

#include <map>
#include <thread>

namespace hako::core::rpc {
    class HakoInternalRpc {
    public:
        HakoInternalRpc(HakoAssetIdType asset_id, std::shared_ptr<data::HakoMasterData> master_data)
        {
            this->asset_id_ = asset_id;
            this->master_data_ = master_data;
            this->proxy_thread_ = nullptr;
        }
        virtual ~HakoInternalRpc()
        {
            this->stop();
            this->proxy_thread_ = nullptr;
            this->master_data_ = nullptr;
        }
        void start();
        void stop();

        //TODO template?
        void register_callback(hako::data::HakoAssetEventType event_id, void (*callback) ());

    private:
        HakoInternalRpc() {}
        void proxy_thread();

        std::shared_ptr<std::thread> proxy_thread_;
        std::map<hako::data::HakoAssetEventType, void (*) ()> map_;
        HakoAssetIdType asset_id_;
        std::shared_ptr<data::HakoMasterData> master_data_;
    };

    void notify(std::shared_ptr<data::HakoMasterData> master_data, HakoAssetIdType asset_id, hako::data::HakoAssetEventType event_id);

}

#endif /* _HAKO_INTERNAL_RPC_HPP_ */