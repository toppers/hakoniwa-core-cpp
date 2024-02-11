#ifndef _HAKO_SHARED_MEMORY_HPP_
#define _HAKO_SHARED_MEMORY_HPP_

#include "types/hako_types.hpp"
#include "utils/hako_share/impl/hako_mmap.hpp"
#include <map>

namespace hako::utils {

    typedef struct {
        int32_t     shm_id;
        int32_t     sem_id;
        uint32_t    data_size;
        char        data[4];
    } SharedMemoryMetaDataType;
    
    typedef struct {
        SharedMemoryMetaDataType *addr;
        int32_t shm_id; /* for shm */
        HakoMmapObjectType *mmap_obj; /* for mmap */
        int32_t sem_id;
    } SharedMemoryInfoType;

    class HakoSharedMemory {
    public:
        virtual ~HakoSharedMemory() {}

        virtual int32_t create_memory(int32_t key, int32_t size) = 0;
        virtual void* load_memory(int32_t key, int32_t size) = 0;

        virtual void* lock_memory(int32_t key) = 0;
        virtual void unlock_memory(int32_t key) = 0;
        virtual void destroy_memory(int32_t key) = 0;

        virtual int32_t get_semid(int32_t key) = 0;

    };
}


#endif /* _HAKO_SHARED_MEMORY_HPP_ */