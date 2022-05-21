#include "core/simulation/time/hako_time.hpp"

HakoTimeType hako::core::simulation::time::TheWorld::time_begins_to_move(std::vector<HakoTimeType> &asset_times)
{
    bool canStep = true;

    for (HakoTimeType &asset_time : asset_times)
    {
        HakoTimeType diff = asset_time - this->world_time_usec_;
        if (diff <= -this->max_delay_time_usec_)
        {
            canStep = false;
            break;
        }
    }
    if (canStep) {
        this->world_time_usec_ += this->delta_time_usec_;
    }
    else {
        this->wait_time_usec_ += this->delta_time_usec_;
    }
    return this->world_time_usec_;
}