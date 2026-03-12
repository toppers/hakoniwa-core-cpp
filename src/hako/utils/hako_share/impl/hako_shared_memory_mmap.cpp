#include "types/hako_types.hpp"

#include "utils/hako_share/hako_shared_memory_mmap.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"
#include "utils/hako_share/impl/hako_mmap.hpp"
#include "utils/hako_config_loader.hpp"
#include <sys/stat.h>

namespace {
static std::string get_core_filepath(int32_t key)
{
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
    return std::string(buf);
}
static hako::utils::SharedMemoryInfoType* find_memory_info(
    std::map<int32_t, hako::utils::SharedMemoryInfoType>& shared_memory_map,
    int32_t key)
{
    auto it = shared_memory_map.find(key);
    if (it == shared_memory_map.end()) {
        printf("ERROR: shared memory key not found: key=%d\n", key);
        return nullptr;
    }
    return &it->second;
}
}

int32_t hako::utils::HakoSharedMemoryMmap::create_memory(int32_t key, int32_t size)
{
    int32_t total_size = size + sizeof(SharedMemoryMetaDataType);
    std::string filepath = get_core_filepath(key);

    HakoMmapObjectType *mmap_obj = hako_mmap_create(filepath, total_size);
    if (mmap_obj == nullptr) {
        printf("ERROR: hako_mmap_create() id=%d size=%d error=%d\n", key, size, errno);
        return -1;
    }
    //printf("INFO: hako_mmap_create() key=%d size=%d \n", key, size);
    void *shared_memory = mmap_obj->mmap_addr;

    int32_t sem_id = hako::utils::sem::flock::create(key);
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
void* hako::utils::HakoSharedMemoryMmap::load_memory(int32_t key, int32_t size)
{
    if (size == 0) {
        return nullptr;
    }
    return this->load_memory_shmid(key, -1);
}

void* hako::utils::HakoSharedMemoryMmap::load_memory_shmid(int32_t key, int32_t shmid)
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
        info.mmap_obj = mmap_obj;
        info.sem_id = metap->sem_id;
        this->shared_memory_map_.insert(std::make_pair(key, info));
    }
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return nullptr;
    }
    return &info->addr->data[0];
}

void* hako::utils::HakoSharedMemoryMmap::lock_memory(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return nullptr;
    }
    hako::utils::sem::flock::master_lock(info->sem_id);
    return &info->addr->data[0];
}

void hako::utils::HakoSharedMemoryMmap::unlock_memory(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return;
    }
    hako::utils::sem::flock::master_unlock(info->sem_id);
    return;
}
int32_t hako::utils::HakoSharedMemoryMmap::get_semid(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return -1;
    }
    return info->sem_id;
}

void hako::utils::HakoSharedMemoryMmap::destroy_memory(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return;
    }
    void *addr = info->addr;
    if (addr != nullptr) {
        if (info->mmap_obj != nullptr) {
            hako_mmap_destroy(info->mmap_obj);
        }
        hako::utils::sem::flock::destroy(info->sem_id);
        this->shared_memory_map_.erase(key);
    }
    return;
}
