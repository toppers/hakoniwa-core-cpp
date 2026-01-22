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
        virtual bool asset_register(const std::string & name, AssetCallbackType &callbacks) override;
        virtual bool asset_register_polling(const std::string & name) override;
        virtual HakoSimulationAssetEventType asset_get_event(const std::string & name) override;
        virtual bool asset_unregister(const std::string & name) override;
        virtual void notify_simtime(const std::string & name, HakoTimeType simtime) override;
        virtual HakoTimeType get_worldtime() override;
        virtual HakoTimeType get_min_asset_time() override;

        /*
         * feedback events
         */
        bool start_feedback(const std::string& asset_name, bool isOk) override;
        bool stop_feedback(const std::string& asset_name, bool isOk) override;
        bool reset_feedback(const std::string& asset_name, bool isOk) override;

        /*
         * PDU APIs
         */
        bool create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size) override;
        bool create_pdu_lchannel(const std::string& robo_name, HakoPduChannelIdType channel_id, size_t pdu_size) override;
        HakoPduChannelIdType get_pdu_channel(const std::string& robo_name, HakoPduChannelIdType channel_id) override;
        bool is_pdu_dirty(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id) override;
        bool write_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len) override;
        bool write_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len) override;
        bool read_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len) override;
        bool read_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len) override;
        void notify_read_pdu_done(const std::string& asset_name) override;
        void notify_write_pdu_done(const std::string& asset_name) override;
        bool is_pdu_sync_mode(const std::string& asset_name) override;
        bool is_simulation_mode() override;
        bool is_pdu_created()  override;

        bool write_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len)  override;
        bool read_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len)  override;

#ifdef HAKO_CORE_EXTENSION
        bool register_asset_extension(std::shared_ptr<hako::extension::IHakoAssetExtension> asset_extension) override
        {
            this->master_data_->register_asset_extension(asset_extension);
            return true;
        }
        std::shared_ptr<extension::IHakoAssetExtension> get_asset_extension() override
        {
            return this->master_data_->get_asset_extension();
        }
#endif
    private:
        HakoAssetControllerImpl() {}
        bool feedback(const std::string& asset_name, bool isOk, HakoSimulationStateType exp_state);
        std::shared_ptr<data::HakoMasterData> master_data_;
        std::shared_ptr<hako::core::rpc::HakoInternalRpc> rpc_;
    };
}

#endif /* _HAKO_ASSET_IMPL_HPP_ */
