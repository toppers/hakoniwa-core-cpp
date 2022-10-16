#ifndef _HAKO_SIMEVENT_HPP_
#define _HAKO_SIMEVENT_HPP_

#include "types/hako_types.hpp"

namespace hako {

    class IHakoSimulationEventController {
    public:
        virtual ~IHakoSimulationEventController() {}

        virtual HakoSimulationStateType state() = 0;
        /*
         * simulation execution event
         */
        virtual bool start() = 0;
        virtual bool stop() = 0;
        virtual bool reset() = 0;
        virtual bool assets(std::vector<std::shared_ptr<std::string>> & asset_list) = 0;
        /*
         * PDU inspectors
         */
        virtual bool write_pdu(HakoPduChannelIdType channel_id, const char *pdu_data, size_t len) = 0;
        virtual bool read_pdu(HakoPduChannelIdType channel_id, char *pdu_data, size_t len) = 0;
        virtual size_t pdu_size(HakoPduChannelIdType channel_id) = 0;
    };
}

#endif /* _HAKO_SIMEVENT_HPP_ */