#ifndef _HAKO_MASTER_HPP_
#define _HAKO_MASTER_HPP_

#include "types/hako_types.hpp"

namespace hako {

    class IHakoMasterController {
    public:
        virtual ~IHakoMasterController() {}

        /*
         * Master APIs
         */
        virtual bool execute() = 0;
        virtual void set_config_simtime(HakoTimeType max_delay_time_usec, HakoTimeType delta_time_usec) = 0;
        virtual HakoTimeType get_max_deltay_time_usec() = 0;
        virtual HakoTimeType get_delta_time_usec() = 0;
    };
}

#endif /* _HAKO_MASTER_HPP_ */