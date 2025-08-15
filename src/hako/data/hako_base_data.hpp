#ifndef _HAKO_BASE_DATA_HPP_
#define _HAKO_BASE_DATA_HPP_

#include "types/hako_types.hpp"
#include <atomic>
#include <cstdint>
#include <thread>
#if defined(_WIN32)
  #include <windows.h>
  #include <immintrin.h>
#endif

static inline void hako_cpu_relax_backoff(int& spins) {
#if defined(_MSC_VER)
  _mm_pause();
  if (++spins >= 64) { spins = 0; SwitchToThread(); }
#elif defined(__x86_64__) || defined(__i386__)
  __builtin_ia32_pause();
  if (++spins >= 64) { spins = 0; std::this_thread::yield(); }
#else
  if (++spins >= 64) { spins = 0; std::this_thread::yield(); }
#endif
}

static inline bool hako_atomic_load_bool(const bool* p) {
#if __cpp_lib_atomic_ref
  std::atomic_ref<const bool> a(*p);
  return a.load(std::memory_order_acquire);
#else
  // GCC/Clangなら __atomic_load_n でもOK
  return __atomic_load_n(p, __ATOMIC_ACQUIRE);
#endif
}

static inline void hako_atomic_store_bool(bool* p, bool v) {
#if __cpp_lib_atomic_ref
  std::atomic_ref<bool> a(*p);
  a.store(v, std::memory_order_release);
#else
  __atomic_store_n(p, v, __ATOMIC_RELEASE);
#endif
}

static inline uint32_t hako_atomic_load_u32(const uint32_t* p) {
#if __cpp_lib_atomic_ref
  std::atomic_ref<const uint32_t> a(*p);
  return a.load(std::memory_order_acquire);
#else
  return __atomic_load_n(p, __ATOMIC_ACQUIRE);
#endif
}

static inline void hako_atomic_store_u32(uint32_t* p, uint32_t v) {
#if __cpp_lib_atomic_ref
  std::atomic_ref<uint32_t> a(*p);
  a.store(v, std::memory_order_release);
#else
  __atomic_store_n(p, v, __ATOMIC_RELEASE);
#endif
}

static inline uint32_t hako_atomic_fetch_add_u32(uint32_t* p, uint32_t inc = 1) {
#if __cpp_lib_atomic_ref
  std::atomic_ref<uint32_t> a(*p);
  return a.fetch_add(inc, std::memory_order_release);
#else
  return __atomic_fetch_add(p, inc, __ATOMIC_RELEASE);
#endif
}
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
        bool                atomic_is_dirty[HAKO_PDU_CHANNEL_MAX];
        bool                atomic_is_rbusy[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
        bool                atomic_is_rbusy_for_external[HAKO_PDU_CHANNEL_MAX];
        bool                atomic_is_wbusy[HAKO_PDU_CHANNEL_MAX];
        HakoPduChannelType  channel[HAKO_PDU_CHANNEL_MAX];

        /*
         * usecase: dirty check 
         */
        uint32_t            atomic_pdu_read_version[HAKO_DATA_MAX_ASSET_NUM][HAKO_PDU_CHANNEL_MAX];
        uint32_t            atomic_pdu_write_version[HAKO_PDU_CHANNEL_MAX];
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