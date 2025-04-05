#pragma once

namespace hako::extension {
    class IHakoAssetExtension {
        public:
            virtual ~IHakoAssetExtension() {}
            virtual bool on_pdu_data_write(int real_channel_id) = 0;
    };
    
    class IHakoMasterExtension {
        public:
            virtual ~IHakoMasterExtension() {}
            virtual bool on_pdu_data_create() = 0;
            virtual bool on_pdu_data_load() = 0;
            virtual bool on_pdu_data_reset() = 0;
            virtual bool on_pdu_data_destroy() = 0;
    };
}