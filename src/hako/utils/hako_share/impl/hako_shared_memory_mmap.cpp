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

int32_t hako::utils::HakoSharedMemoryMmap::create_memory(int32_t key, size_t size)
{
    if (size > std::numeric_limits<size_t>::max() - sizeof(SharedMemoryMetaDataType)) {
        printf("ERROR: create_memory() size overflow key=%d size=%zu\n", key, size);
        return -1;
    }

    size_t total_size = size + sizeof(SharedMemoryMetaDataType);
    std::string filepath = get_core_filepath(key);

    HakoMmapObjectType *mmap_obj = hako_mmap_create(filepath, total_size);
    if (mmap_obj == nullptr) {
        printf("ERROR: hako_mmap_create() id=%d size=%zu total_size=%zu error=%d\n",
               key, size, total_size, errno);
        return -1;
    }

    void *shared_memory = mmap_obj->mmap_addr;

    int32_t sem_id = hako::utils::sem::flock::create(key);
    if (sem_id < 0) {
        hako_mmap_destroy(mmap_obj);
        return -1;
    }

    SharedMemoryMetaDataType *metap = static_cast<SharedMemoryMetaDataType*>(shared_memory);
    metap->magic = HAKO_SHM_MAGIC;
    metap->version = HAKO_SHM_LAYOUT_VERSION;
    metap->sem_id = sem_id;
    metap->shm_id = -1;

    if (size > std::numeric_limits<decltype(metap->data_size)>::max()) {
        printf("ERROR: metadata data_size overflow key=%d size=%zu\n", key, size);
        hako_mmap_destroy(mmap_obj);
        return -1;
    }
    metap->data_size = size;
    std::cout
        << "INFO: HakoSharedMemoryMmap::create_memory()"
        << " key=" << key
        << " magic=0x" << std::hex << metap->magic << std::dec
        << " version=" << metap->version
        << " data_size=" << metap->data_size
        << " total_size=" << total_size
        << std::endl;

    SharedMemoryInfoType info;
    info.addr = metap;
    info.shm_id = -1;
    info.mmap_obj = mmap_obj;
    info.sem_id = sem_id;
    this->shared_memory_map_.insert(std::make_pair(key, info));
    return 0;
}
void* hako::utils::HakoSharedMemoryMmap::load_memory(int32_t key, size_t size)
{
    if (size == 0) {
        return nullptr;
    }
    SharedMemoryMetaDataType *metap =
        static_cast<SharedMemoryMetaDataType*>(this->load_memory_shmid(key, -1));

    if (metap == nullptr) {
        std::cout << "ERROR: load_memory() load_memory_shmid() return nullptr key=" << key << " size=" << size << std::endl;
        return nullptr;
    }

    if (metap->magic != HAKO_SHM_MAGIC) {
        std::cout << "ERROR: shared memory magic mismatch key=" << key << " magic=" << metap->magic << " expected=" << HAKO_SHM_MAGIC << std::endl;
        return nullptr;
    }

    if (metap->version != HAKO_SHM_LAYOUT_VERSION) {
        std::cout << "ERROR: shared memory layout version mismatch key=" << key << " version=" << metap->version << " expected=" << HAKO_SHM_LAYOUT_VERSION << std::endl;
        return nullptr;
    }
    if (metap->data_size != static_cast<uint64_t>(size)) {
        std::cout
            << "ERROR: shared memory data_size mismatch"
            << " key=" << key
            << " data_size=" << metap->data_size
            << " requested_size=" << size
            << std::endl;
        return nullptr;
    }    
    std::cout
        << "INFO: HakoSharedMemoryMmap::load_memory()"
        << " key=" << key
        << " magic=0x" << std::hex << metap->magic << std::dec
        << " version=" << metap->version
        << " data_size=" << metap->data_size
        << " requested_size=" << size
        << std::endl;

    return static_cast<void*>(metap);
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
