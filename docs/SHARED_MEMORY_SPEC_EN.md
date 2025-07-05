# Hakoniwa Shared Memory Data Structure Specification

The Hakoniwa core library shares state and PDU data between the master and assets via shared memory. This document describes that layout.

## Shared Memory IDs

The following IDs are defined in `hako_config.hpp`.

- `HAKO_SHARED_MEMORY_ID_0` : master information (`HakoMasterDataType`)
- `HAKO_SHARED_MEMORY_ID_1` : PDU data area (`HakoPduData`)
- `HAKO_SHARED_MEMORY_ID_2` : reserved for extension

## HakoMasterDataType

This structure is placed in the master shared memory (`HAKO_SHARED_MEMORY_ID_0`).

```cpp
struct HakoMasterDataType {
    pid_type                master_pid;
    HakoSimulationStateType state;
    HakoTimeSetType         time_usec;
    uint32_t                asset_num;
    HakoAssetEntryType      assets[HAKO_DATA_MAX_ASSET_NUM];
    HakoAssetEntryEventType assets_ev[HAKO_DATA_MAX_ASSET_NUM];
    HakoPduMetaDataType     pdu_meta_data;
    HakoLogDataType         log;
};
```

### Field overview

- **master_pid** : master process PID
- **state** : simulation state
- **time_usec** : time information holding `max_delay`, `delta` and `current`
- **asset_num** : number of registered assets
- **assets[]** : static asset information (ID, name, type, callbacks)
- **assets_ev[]** : dynamic asset info (PID, events, heartbeat, etc.)
- **pdu_meta_data** : PDU channel management information (see below)
- **log** : ring buffer log in shared memory

## HakoPduMetaDataType

Used inside `HakoMasterDataType` to manage placement and access of PDU data (`HAKO_SHARED_MEMORY_ID_1`).

```cpp
struct HakoPduMetaDataType {
    HakoTimeModeType    mode;
    HakoAssetIdType     asset_num;
    HakoAssetIdType     pdu_sync_asset_id;
    bool                asset_pdu_check_status[HAKO_DATA_MAX_ASSET_NUM];
    int32_t             channel_num;
    bool                is_dirty[HAKO_PDU_CHANNEL_MAX];
    bool                is_rbusy[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
    bool                is_rbusy_for_external[HAKO_PDU_CHANNEL_MAX];
    bool                is_wbusy[HAKO_PDU_CHANNEL_MAX];
    HakoPduChannelType  channel[HAKO_PDU_CHANNEL_MAX];
    uint32_t            pdu_read_version[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
    uint32_t            pdu_write_version[HAKO_PDU_CHANNEL_MAX];
    HakoPduChannelMapType channel_map[HAKO_PDU_CHANNEL_MAX];
};
```

The fields serve the following purposes.

- **mode** : indicates master-driven or asset-driven mode
- **asset_num** : number of assets participating in PDU sync
- **pdu_sync_asset_id** : asset ID used for synchronization
- **asset_pdu_check_status[]** : per-asset flag for PDU transmission confirmation
- **channel_num** : number of PDU channels in use
- **is_dirty[]** : per-channel update flag
- **is_rbusy[][] / is_wbusy[]** : read/write busy flags
- **channel[]** : `offset` and `size` for each channel
- **pdu_read_version[][] / pdu_write_version[]** : version counters for synchronization
- **channel_map[]** : logical channel mapping

## PDU Data Area

Actual PDU bytes are allocated in `HAKO_SHARED_MEMORY_ID_1`. They are placed consecutively according to `pdu_meta_data.channel[i].offset` and `size` when `HakoPduData::create()` is called.

```
offset = channel[i].offset
size   = channel[i].size
pdu_memory[offset .. offset + size - 1] contains data for channel i
```

The region is initialized by `HakoPduData::reset()` when resetting. The read/write APIs reference flags such as `is_rbusy` and `is_wbusy` for mutual exclusion.

## Log Buffer

`HakoLogDataType` at the end of `HakoMasterDataType` is a fixed length log area that keeps `HAKO_LOGGER_LOGNUM` entries in a ring buffer manner.

## References

- `src/hako/data/hako_base_data.hpp`
- `src/hako/data/hako_master_data.hpp`
- `src/hako/data/hako_pdu_data.hpp`
