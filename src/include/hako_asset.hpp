#ifndef _HAKO_ASSET_HPP_
#define _HAKO_ASSET_HPP_

#include <string>
#include "types/hako_types.hpp"
#ifdef HAKO_CORE_EXTENSION
#include <memory>
#include "hako_extension.hpp"
#endif
namespace hako {

    class IHakoAssetController {
    public:
        virtual ~IHakoAssetController() {}

        /*
         * Asset APIs
         */
        virtual bool asset_register(const std::string & name, AssetCallbackType &callbacks) = 0;      
        virtual bool asset_register_polling(const std::string & name) = 0;      
        virtual HakoSimulationAssetEventType asset_get_event(const std::string & name) = 0;      
        virtual bool asset_unregister(const std::string & name) = 0;
        virtual void notify_simtime(const std::string & name, HakoTimeType simtime) = 0;
        virtual HakoTimeType get_worldtime() = 0;
        virtual HakoTimeType get_min_asset_time() = 0;


        /*
         * feedback events
         */
        virtual bool start_feedback(const std::string& asset_name, bool isOk) = 0;
        virtual bool stop_feedback(const std::string& asset_name, bool isOk) = 0;
        virtual bool reset_feedback(const std::string& asset_name, bool isOk) = 0;

        /*
         * PDU APIs
         */
        virtual bool create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size) = 0;
        virtual bool create_pdu_lchannel(const std::string& robo_name, HakoPduChannelIdType channel_id, size_t pdu_size) = 0;
        virtual HakoPduChannelIdType get_pdu_channel(const std::string& robo_name, HakoPduChannelIdType channel_id) = 0;
        virtual bool is_pdu_dirty(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id) = 0;
        virtual bool write_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len) = 0;
        virtual bool write_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len) = 0;
        virtual bool read_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len) = 0;
        virtual bool read_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len) = 0;
        virtual void notify_read_pdu_done(const std::string& asset_name) = 0;
        virtual void notify_write_pdu_done(const std::string& asset_name) = 0;
        virtual bool is_pdu_sync_mode(const std::string& asset_name) = 0;
        virtual bool is_simulation_mode() = 0;
        virtual bool is_pdu_created() = 0;

        virtual bool write_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, const char *pdu_data, size_t len) = 0;
        virtual bool read_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, char *pdu_data, size_t len) = 0;
        //TODO
        //get asset lists
#ifdef HAKO_CORE_EXTENSION
        virtual bool register_asset_extension(std::shared_ptr<extension::IHakoAssetExtension> extension) = 0;
        virtual std::shared_ptr<extension::IHakoAssetExtension> get_asset_extension() = 0;
#endif
    };
}

#endif /* _HAKO_ASSET_HPP_ */
