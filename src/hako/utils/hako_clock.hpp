#ifndef _HAKO_CLOCK_HPP_
#define _HAKO_CLOCK_HPP_

#include <types/hako_types.hpp>
#include <utils/hako_logger.hpp>
#include <time.h>

static inline HakoTimeType hako_get_clock()
{
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return ( (tp.tv_sec * 1000 * 1000) + (tp.tv_nsec / 1000) );
}
static inline bool hako_clock_is_timeout(HakoTimeType check_time, HakoTimeType timeout)
{
    HakoTimeType ctime = hako_get_clock();
    if ((check_time + timeout) <= ctime) {
        //hako::utils::logger::get()->info("TIMEOUT:check_time={0} ctime={1} timeout={2}", check_time, ctime, timeout);
        return true;
    }
    else {
        //hako::utils::logger::get()->info("NONE:check_time={0} ctime={1} timeout={2}", check_time, ctime, timeout);
        return false;
    }
}
#endif /* _HAKO_CLOCK_HPP_ */