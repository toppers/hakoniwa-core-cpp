# Hakoniwa C++ Core API Specification

This document describes the API of the Hakoniwa core library for C++ provided in this repository.

## Basics
- Include header `hako.hpp` to use the functionality.
- Use the `hako` namespace.

## Initialization / Shutdown
| Function | Description |
| --- | --- |
| `bool hako::init()` | Initialize the library. It prepares shared memory. |
| `void hako::destroy()` | Releases resources allocated by `init()`. |

## Creating controller classes
| Function | Description |
| --- | --- |
| `std::shared_ptr<IHakoMasterController> hako::create_master()` | Create a master controller object. |
| `std::shared_ptr<IHakoAssetController> hako::create_asset_controller()` | Create an asset controller object. |
| `std::shared_ptr<IHakoSimulationEventController> hako::get_simevent_controller()` | Get the simulation event controller. |

## IHakoMasterController
This interface is defined in `hako_master.hpp`. Main members are:

| Method | Description |
| --- | --- |
| `bool execute()` | Execute one simulation step. |
| `void set_config_simtime(HakoTimeType max_delay_time_usec, HakoTimeType delta_time_usec)` | Configure simulation time parameters. |
| `HakoTimeType get_max_deltay_time_usec()` | Get the configured maximum delay time in microseconds. |
| `HakoTimeType get_delta_time_usec()` | Get the time tick in microseconds. |

## IHakoAssetController
This interface is defined in `hako_asset.hpp`. Key APIs are as follows.

- Asset management
  - `bool asset_register(const std::string& name, AssetCallbackType& callbacks)`
  - `bool asset_register_polling(const std::string& name)`
  - `HakoSimulationAssetEventType asset_get_event(const std::string& name)`
  - `bool asset_unregister(const std::string& name)`
  - `void notify_simtime(const std::string& name, HakoTimeType simtime)`
  - `HakoTimeType get_worldtime()`

- Feedback events
  - `bool start_feedback(const std::string& asset_name, bool isOk)`
  - `bool stop_feedback(const std::string& asset_name, bool isOk)`
  - `bool reset_feedback(const std::string& asset_name, bool isOk)`

- PDU communication
  - `bool create_pdu_channel(HakoPduChannelIdType channel_id, size_t pdu_size)`
  - `bool create_pdu_lchannel(const std::string& robo_name, HakoPduChannelIdType channel_id, size_t pdu_size)`
  - `HakoPduChannelIdType get_pdu_channel(const std::string& robo_name, HakoPduChannelIdType channel_id)`
  - `bool is_pdu_dirty(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id)`
  - `bool write_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)`
  - `bool write_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)`
  - `bool read_pdu(const std::string& asset_name, const std::string& robo_name, HakoPduChannelIdType channel_id, char* pdu_data, size_t len)`
  - `bool read_pdu_for_external(const std::string& robo_name, HakoPduChannelIdType channel_id, char* pdu_data, size_t len)`
  - `void notify_read_pdu_done(const std::string& asset_name)`
  - `void notify_write_pdu_done(const std::string& asset_name)`
  - `bool is_pdu_sync_mode(const std::string& asset_name)`
  - `bool is_simulation_mode()`
  - `bool is_pdu_created()`
  - `bool write_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)`
  - `bool read_pdu_nolock(const std::string& robo_name, HakoPduChannelIdType channel_id, char* pdu_data, size_t len)`

## IHakoSimulationEventController
This interface is defined in `hako_simevent.hpp`.

| Method | Description |
| --- | --- |
| `HakoSimulationStateType state()` | Get the current simulation state. |
| `bool start()` | Issue a simulation start event. |
| `bool stop()` | Issue a simulation stop event. |
| `bool reset()` | Issue a simulation reset event. |
| `bool assets(std::vector<std::shared_ptr<std::string>>& asset_list)` | Get the list of registered asset names. |
| `HakoPduChannelIdType get_pdu_channel(const std::string& asset_name, HakoPduChannelIdType channel_id)` | Get the PDU channel ID for the specified asset. |
| `bool write_pdu(HakoPduChannelIdType channel_id, const char* pdu_data, size_t len)` | Write PDU data. |
| `bool read_pdu(HakoPduChannelIdType channel_id, char* pdu_data, size_t len)` | Read PDU data. |
| `size_t pdu_size(HakoPduChannelIdType channel_id)` | Get the PDU size for the specified channel. |
| `void print_master_data()` | Print master information to standard output. |
| `void print_memory_log()` | Print logs stored in shared memory. |

## Major Data Types
The main types defined in `types/hako_types.hpp` are:

- `HakoTimeType` : 64bit integer representing time in microseconds.
- `HakoAssetIdType` : 32bit integer representing an asset ID.
- `HakoPduChannelIdType` : 32bit integer representing a PDU channel ID.
- `HakoSimulationStateType` : enumeration for simulation state.
- `HakoSimulationAssetEventType` : enumeration for asset events.
- `HakoFixedStringType` : fixed length string struct.
- `AssetCallbackType` : structure holding start/stop/reset callbacks of an asset.

## Logging
`hako_log.hpp` provides the following macros:

- `HAKO_LOG_INFO(...)`
- `HAKO_LOG_WARN(...)`
- `HAKO_LOG_ERROR(...)`

Internally they call `hako::log::add()` to append a log to shared memory. Be careful not to call them from locked sections to avoid deadlock.

## Constants
Configuration values are defined in `config/hako_config.hpp`. Main constants:

- `HAKO_FIXED_STRLEN_MAX` : maximum length of fixed strings.
- `HAKO_DATA_MAX_ASSET_NUM` : maximum number of assets.
- `HAKO_PDU_CHANNEL_MAX` : maximum number of PDU channels.
- `HAKO_ASSET_TIMEOUT_USEC` : asset timeout value.

## Sample
Basic implementations of master, asset and command tools can be found under `sample/base-procs`. Use these as references for real applications.
