#ifndef _HAKO_TYPES_HPP_
#define _HAKO_TYPES_HPP_

#include <vector>
#include <cstdint>
#include <memory>
#include "types/hako_osdeps.hpp"
#include "config/hako_config.hpp"

/*
 * usec
 */
typedef int64_t HakoTimeType;

typedef int32_t HakoAssetIdType;
typedef int32_t HakoPduChannelIdType;

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

typedef enum
{
    HakoSimAssetEvent_None = 0,
    HakoSimAssetEvent_Start,
    HakoSimAssetEvent_Stop,
    HakoSimAssetEvent_Reset,
    HakoSimAssetEvent_Error,
    HakoSimAssetEvent_Count
} HakoSimulationAssetEventType;

#endif /* _HAKO_TYPES_HPP_ */
