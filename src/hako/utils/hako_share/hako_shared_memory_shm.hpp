#ifndef _HAKO_SHARED_MEMORY_SHM_HPP_
#define _HAKO_SHARED_MEMORY_SHM_HPP_

#include "types/hako_types.hpp"
#include "utils/hako_share/hako_shared_memory.hpp"
#include <map>

namespace hako::utils {

    class HakoSharedMemoryShm : public HakoSharedMemory {
    public:
        HakoSharedMemoryShm() {}
        virtual ~HakoSharedMemoryShm() {}

        virtual int32_t create_memory(int32_t key, int32_t size) override;
        virtual void* load_memory(int32_t key, int32_t size) override;

        virtual void* lock_memory(int32_t key) override;
        virtual void unlock_memory(int32_t key) override;
        virtual void destroy_memory(int32_t key) override;

        virtual int32_t get_semid(int32_t key) override;

    private:
        void* load_memory_shmid(int32_t key, int32_t shmid);
        std::map<int32_t, SharedMemoryInfoType> shared_memory_map_;
    };
}


#endif /* _HAKO_SHARED_MEMORY_SHM_HPP_ */