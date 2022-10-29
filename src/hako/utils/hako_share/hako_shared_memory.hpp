#ifndef _HAKO_SHARED_MEMORY_HPP_
#define _HAKO_SHARED_MEMORY_HPP_

#include "types/hako_types.hpp"
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
        int32_t shm_id;
        int32_t sem_id;
    } SharedMemoryInfoType;

    class HakoSharedMemory {
    public:
        HakoSharedMemory() {}
        virtual ~HakoSharedMemory() {}

        int32_t create_memory(int32_t key, int32_t size);
        void* load_memory(int32_t key, int32_t size);

        void* lock_memory(int32_t key);
        void unlock_memory(int32_t key);
        void destroy_memory(int32_t key);

        int32_t get_shmid();
        int32_t get_semid(int32_t key);

    private:
        void* load_memory_shmid(int32_t key, int32_t shmid);
        std::map<int32_t, SharedMemoryInfoType> shared_memory_map_;
    };
}


#endif /* _HAKO_SHARED_MEMORY_HPP_ */