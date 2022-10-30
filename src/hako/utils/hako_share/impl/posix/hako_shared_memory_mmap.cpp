#include "types/hako_types.hpp"

#include "utils/hako_share/hako_shared_memory.hpp"
#include "utils/hako_share/hako_sem.hpp"
#include "utils/hako_share/impl/hako_mmap.hpp"
#include "utils/hako_config_loader.hpp"

int32_t hako::utils::HakoSharedMemory::create_memory(int32_t key, int32_t size)
{
    int32_t total_size = size + sizeof(SharedMemoryMetaDataType);
    char buf[4096];
    HakoConfigType config;
    hako_config_load(config);

    if (config.param == nullptr) {
        snprintf(buf, sizeof(buf), "./mmap-0x%x.bin", key);
    }
    else {
        std::string core_mmap_path = config.param["core_mmap_path"];
        snprintf(buf, sizeof(buf), "%s/mmap-0x%x.bin", core_mmap_path.c_str(), key);
    }
    //printf("INFO: mmap path=%s\n", buf);
    std::string filepath(buf);
    HakoMmapObjectType *mmap_obj = hako_mmap_create(filepath, total_size);
    if (mmap_obj == nullptr) {
        printf("ERROR: hako_mmap_create() id=%d size=%d error=%d\n", key, size, errno);
        return -1;
    }
    //printf("INFO: hako_mmap_create() key=%d size=%d \n", key, size);
    void *shared_memory = mmap_obj->mmap_addr;

    int32_t sem_id = hako::utils::sem::create(key);
    if (sem_id < 0) {
        hako_mmap_destroy(mmap_obj);
        return -1;
    }

    SharedMemoryMetaDataType *metap = static_cast<SharedMemoryMetaDataType*>(shared_memory);
    metap->sem_id = sem_id;
    metap->shm_id = -1;
    metap->data_size = size;
    SharedMemoryInfoType info;
    info.addr = metap;
    info.shm_id = -1;
    info.mmap_obj = mmap_obj;
    info.sem_id = sem_id;
    this->shared_memory_map_.insert(std::make_pair(key, info));
    return 0;
}
void* hako::utils::HakoSharedMemory::load_memory(int32_t key, int32_t size)
{
    if (size == 0) {
        return nullptr;
    }
    return this->load_memory_shmid(key, -1);
}

void* hako::utils::HakoSharedMemory::load_memory_shmid(int32_t key, int32_t shmid)
{
    if (shmid >= 0) {
        return nullptr;
    }
    if (this->shared_memory_map_.count(key) == 0) {
        char buf[4096];
        HakoConfigType config;
        hako_config_load(config);
        if (config.param == nullptr) {
            snprintf(buf, sizeof(buf), "./mmap-0x%x.bin", key);
        }
        else {
            std::string core_mmap_path = config.param["core_mmap_path"];
            snprintf(buf, sizeof(buf), "%s/mmap-0x%x.bin", core_mmap_path.c_str(), key);
        }
        //printf("INFO: mmap path=%s\n", buf);
        std::string filepath(buf);

        HakoMmapObjectType *mmap_obj = hako_mmap_open(filepath);
        if (mmap_obj == nullptr) {
            printf("ERROR: hako_mmap_open() id=%d error=%d\n", key, errno);
            return nullptr;
        }
        void *shared_memory = mmap_obj->mmap_addr;
        SharedMemoryMetaDataType *metap = static_cast<SharedMemoryMetaDataType*>(shared_memory);
        SharedMemoryInfoType info;
        info.addr = metap;
        info.shm_id = metap->shm_id;
        info.sem_id = metap->sem_id;
        this->shared_memory_map_.insert(std::make_pair(key, info));
    }
    return &this->shared_memory_map_[key].addr->data[0];
}

void* hako::utils::HakoSharedMemory::lock_memory(int32_t key)
{
    hako::utils::sem::master_lock(this->shared_memory_map_[key].sem_id);
    return &this->shared_memory_map_[key].addr->data[0];
}

void hako::utils::HakoSharedMemory::unlock_memory(int32_t key)
{
    hako::utils::sem::master_unlock(this->shared_memory_map_[key].sem_id);
    return;
}
int32_t hako::utils::HakoSharedMemory::get_semid(int32_t key)
{
    return this->shared_memory_map_[key].sem_id;
}

void hako::utils::HakoSharedMemory::destroy_memory(int32_t key)
{
    void *addr = this->shared_memory_map_[key].addr;
    if (addr != nullptr) {
        hako_mmap_destroy(this->shared_memory_map_[key].mmap_obj);
        hako::utils::sem::destroy(this->shared_memory_map_[key].sem_id);
        this->shared_memory_map_.erase(key);
    }
    return;
}