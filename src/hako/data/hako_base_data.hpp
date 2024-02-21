#ifndef _HAKO_BASE_DATA_HPP_
#define _HAKO_BASE_DATA_HPP_

#include "types/hako_types.hpp"

namespace hako::data {

    typedef struct {
        HakoTimeType    max_delay;
        HakoTimeType    delta;
        HakoTimeType    current;
    } HakoTimeSetType;

    typedef enum {
        HakoAsset_Unknown = 0,
        HakoAsset_Inside,
        HakoAsset_Outside,
        HakoAsset_Count
    } HakoAssetType;


    typedef struct {
        HakoAssetIdType     id;
        HakoFixedStringType name;
        HakoAssetType       type;
        AssetCallbackType   callback;
    } HakoAssetEntryType;

    typedef enum {
        HakoAssetEvent_None = 0,
        HakoAssetEvent_Start,
        HakoAssetEvent_Stop,
        HakoAssetEvent_Reset,
        HakoAssetEvent_Error,
        HakoAssetEvent_Count
    } HakoAssetEventType;

    typedef struct {
        pid_type            pid;
        HakoTimeType        ctime;        /* usec: for asset simulation time */
        HakoTimeType        update_time;  /* usec: for heartbeat check */
        HakoAssetEventType  event;
        bool                event_feedback;
        int32_t             semid;       /* for remote asset */
    } HakoAssetEntryEventType;

    typedef struct {
        int32_t                 offset;
        size_t                  size;
    } HakoPduChannelType;

    typedef struct {
        HakoFixedStringType robo_name;
        HakoPduChannelIdType logical_channel_id;
    } HakoPduChannelMapType;

    typedef struct {
        char    data[8U];
    } HakoPduChannelDataType;

    typedef enum {
        HakoTimeMode_Master = 0,
        HakoTimeMode_Asset,
        HakoTimeMode_Num
    } HakoTimeModeType;
    typedef struct {
        HakoTimeModeType    mode;
        HakoAssetIdType     asset_num;
        HakoAssetIdType     pdu_sync_asset_id;
        bool                asset_pdu_check_status[HAKO_DATA_MAX_ASSET_NUM];
        int32_t             channel_num;
        bool                is_dirty[HAKO_PDU_CHANNEL_MAX];
        bool                is_rbusy[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
        bool                is_wbusy[HAKO_PDU_CHANNEL_MAX];
        HakoPduChannelType  channel[HAKO_PDU_CHANNEL_MAX];

        /*
         * usecase: dirty check 
         */
        uint32_t            pdu_read_version[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
        uint32_t            pdu_write_version[HAKO_PDU_CHANNEL_MAX];
        /*
         * logical pdu channel management
         */
        HakoPduChannelMapType channel_map[HAKO_PDU_CHANNEL_MAX];
    } HakoPduMetaDataType;

    typedef struct {
        int index;
        char data[HAKO_LOGGER_LOGNUM][HAKO_LOGGER_ENTRYSIZE];
    } HakoLogDataType;
}

#endif /* _HAKO_BASE_DATA_HPP_ */