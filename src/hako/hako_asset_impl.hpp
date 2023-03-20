#ifndef _HAKO_ASSET_IMPL_HPP_
#define _HAKO_ASSET_IMPL_HPP_

#include "hako_asset.hpp"
#include "data/hako_master_data.hpp"
#include "core/context/hako_context.hpp"
#include "core/rpc/hako_internal_rpc.hpp"

namespace hako {
    class HakoAssetControllerImpl : public IHakoAssetController {
    public:
        HakoAssetControllerImpl(std::shared_ptr<data::HakoMasterData> master_data)
        {
            this->master_data_ = master_data;
            this->rpc_ = nullptr;
        }
        virtual ~HakoAssetControllerImpl()
        {
            this->master_data_ = nullptr;
            this->rpc_ = nullptr;
        }
        virtual bool asset_register(const std::string & name, AssetCallbackType &callbacks);
        virtual bool asset_register_polling(const std::string & name);
        virtual HakoSimulationAssetEventType asset_get_event(const std::string & name);
        virtual bool asset_unregister(const std::string & name);
        virtual void notify_simtime(const std::string & name, HakoTimeType simtime);
        virtual HakoTimeType get_worldtime();

        /*
         * feedback events
         */
        bool start_feedback(const std::string& asset_name, bool isOk);
        bool stop_feedback(const std::string& asset_name, bool isOk);
        bool reset_feedback(const std::string& asset_name, bool isOk);

        /*
         * PDU APIs
         */
        bool create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size);
        bool create_pdu_lchannel(const std::string& asset_name, HakoPduChannelIdType channel_id, size_t pdu_size);
        HakoPduChannelIdType get_pdu_channel(const std::string& asset_name, HakoPduChannelIdType channel_id);
        bool is_pdu_dirty(const std::string& asset_name, HakoPduChannelIdType channel_id);
        bool write_pdu(const std::string& asset_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len);
        bool read_pdu(const std::string& asset_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len);
        void notify_read_pdu_done(const std::string& asset_name);
        void notify_write_pdu_done(const std::string& asset_name);
        bool is_pdu_sync_mode(const std::string& asset_name);
        bool is_simulation_mode();
        bool is_pdu_created();

    private:
        HakoAssetControllerImpl() {}
        bool feedback(const std::string& asset_name, bool isOk, HakoSimulationStateType exp_state);
        std::shared_ptr<data::HakoMasterData> master_data_;
        std::shared_ptr<hako::core::rpc::HakoInternalRpc> rpc_;
    };
}

#endif /* _HAKO_ASSET_IMPL_HPP_ */