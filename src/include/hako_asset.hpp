#ifndef _HAKO_ASSET_HPP_
#define _HAKO_ASSET_HPP_

#include <string>
#include "types/hako_types.hpp"

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


        /*
         * feedback events
         */
        virtual bool start_feedback(const std::string& asset_name, bool isOk) = 0;
        virtual bool stop_feedback(const std::string& asset_name, bool isOk) = 0;
        virtual bool reset_feedback(const std::string& asset_name, bool isOk) = 0;

        //TODO
        //get asset lists

    };
}

#endif /* _HAKO_ASSET_HPP_ */