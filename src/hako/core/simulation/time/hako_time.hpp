#ifndef _HAKO_TIME_HPP_
#define _HAKO_TIME_HPP_

#include "types/hako_types.hpp"
#include <vector>

namespace hako::core::simulation::time {

    class TheWorld {
    public:
        TheWorld(HakoTimeType max, HakoTimeType delta) {
            this->max_delay_time_usec_ = max;
            this->delta_time_usec_ = delta;
            this->wait_time_usec_ = 0;
            this->world_time_usec_ = 0;
        }
        ~TheWorld() {}

        HakoTimeType time_begins_to_move(std::vector<HakoTimeType> &asset_times);
        void set_config_simtime(HakoTimeType max_delay_time_usec, HakoTimeType delta_time_usec)
        {
            this->max_delay_time_usec_ = max_delay_time_usec;
            this->delta_time_usec_ = delta_time_usec;
            this->wait_time_usec_ = 0;
            this->world_time_usec_ = 0;
        }
        void reset_world_time()
        {
            this->world_time_usec_ = 0;
        }
        HakoTimeType get_world_time_usec()
        {
            return this->world_time_usec_;
        }
        HakoTimeType get_wait_time_usec()
        {
            return this->wait_time_usec_;
        }
        HakoTimeType get_max_delay_time_usec()
        {
            return this->max_delay_time_usec_;
        }
        HakoTimeType get_delta_time_usec()
        {
            return this->delta_time_usec_;
        }
    private:
        TheWorld() {}
        HakoTimeType max_delay_time_usec_;
        HakoTimeType delta_time_usec_;
        HakoTimeType wait_time_usec_;
        HakoTimeType world_time_usec_;
    };

}

#endif /* _HAKO_TIME_HPP_ */