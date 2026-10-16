#ifndef _HAKO_SIMEVENT_STATES_HPP_
#define _HAKO_SIMEVENT_STATES_HPP_

#include "types/hako_types.hpp"

namespace hako {
    /*
     * The simulation states in which the conductor's event handling has work (the
     * assets' feedback, timeouts) and so takes the master lock (#73). In the others
     * it does nothing; the conductor calls it in a tight loop and must not take the
     * lock there.
     */
    inline bool sim_event_needs_handling(HakoSimulationStateType state)
    {
        switch (state) {
            case HakoSim_Runnable:
            case HakoSim_Stopping:
            case HakoSim_Resetting:
                return true;
            default:
                return false;
        }
    }
}

#endif /* _HAKO_SIMEVENT_STATES_HPP_ */
