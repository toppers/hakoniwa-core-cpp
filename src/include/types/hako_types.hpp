#ifndef _HAKO_TYPES_HPP_
#define _HAKO_TYPES_HPP_

#include <vector>
#include <cstdint>
#include <memory>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <errno.h>
#include "config/hako_config.hpp"

/*
 * usec
 */
typedef int64_t HakoTimeType;

typedef int32_t HakoAssetIdType;
typedef struct {
    uint32_t    len;
    char        data[HAKO_FIXED_STRLEN_MAX + 1];
} HakoFixedStringType;

typedef struct {
    void (*start) ();
    void (*stop) ();
    void (*reset) ();
} AssetCallbackType;

typedef enum
{
    HakoSim_Stopped = 0,
    HakoSim_Runnable,
    HakoSim_Running,
    HakoSim_Stopping,
    HakoSim_Resetting,
    HakoSim_Error,
    HakoSim_Terminated,
    HakoSim_Any,
    HakoSim_Count
} HakoSimulationStateType;


#endif /* _HAKO_TYPES_HPP_ */
