#include "types/hako_types.hpp"

#include "utils/hako_share/hako_shared_memory_shm.hpp"
#include "utils/hako_share/hako_sem.hpp"

namespace {
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

int32_t hako::utils::HakoSharedMemoryShm::create_memory(int32_t key, int32_t size)
{
    int32_t total_size = size + sizeof(SharedMemoryMetaDataType);
    int32_t shmid = shmget(key, total_size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid < 0) {
        printf("ERROR: shmget() id=%d size=%d error=%d\n", key, size, errno);
        return -1;
    }
    printf("INFO: shmget() key=%d size=%d \n", key, size);
    void *shared_memory = shmat(shmid, 0, 0);
    if (shared_memory == ((void*)-1)) {
        printf("ERROR: shmat() id=%d size=%d error=%d\n", key, size, errno);
        return -1;
    }

    int32_t sem_id = hako::utils::sem::create(key);
    if (sem_id < 0) {
        printf("ERROR: hako::utils::sem::create() id=%d size=%d error=%d\n", key, size, errno);
        (void)shmdt(shared_memory);
        (void)shmctl (shmid, IPC_RMID, 0);
        return -1;
    }

    SharedMemoryMetaDataType *metap = static_cast<SharedMemoryMetaDataType*>(shared_memory);
    metap->sem_id = sem_id;
    metap->shm_id = shmid;
    metap->data_size = size;
    SharedMemoryInfoType info;
    info.addr = metap;
    info.shm_id = shmid;
    info.sem_id = sem_id;
    info.mmap_obj = nullptr;
    this->shared_memory_map_.insert(std::make_pair(key, info));
    return shmid;
}
void* hako::utils::HakoSharedMemoryShm::load_memory(int32_t key, int32_t size)
{
    int32_t total_size = size + sizeof(SharedMemoryMetaDataType);
    int32_t shmid = shmget(key, total_size, S_IRUSR | S_IWUSR);
    if (shmid < 0) {
        printf("ERROR: shmget() key=%d size=%d error=%d\n", key, size, errno);
        return nullptr;
    }
    return this->load_memory_shmid(key, shmid);
}

void* hako::utils::HakoSharedMemoryShm::load_memory_shmid(int32_t key, int32_t shmid)
{
    void *shared_memory = shmat(shmid, 0, 0);
    if (shared_memory == ((void*)-1)) {
        printf("ERROR: shmat() key=%d shmid=%d error=%d\n", key, shmid, errno);
        return nullptr;
    }
    SharedMemoryMetaDataType *metap = static_cast<SharedMemoryMetaDataType*>(shared_memory);
    SharedMemoryInfoType info;
    info.addr = metap;
    info.shm_id = metap->shm_id;
    info.sem_id = metap->sem_id;
    info.mmap_obj = nullptr;
    this->shared_memory_map_.insert(std::make_pair(key, info));
    SharedMemoryInfoType *info_ptr = find_memory_info(this->shared_memory_map_, key);
    if (info_ptr == nullptr) {
        return nullptr;
    }
    return &info_ptr->addr->data[0];
}

void* hako::utils::HakoSharedMemoryShm::lock_memory(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return nullptr;
    }
    hako::utils::sem::master_lock(info->sem_id);
    return &info->addr->data[0];
}

void hako::utils::HakoSharedMemoryShm::unlock_memory(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return;
    }
    hako::utils::sem::master_unlock(info->sem_id);
    return;
}
int32_t hako::utils::HakoSharedMemoryShm::get_semid(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return -1;
    }
    return info->sem_id;
}

void hako::utils::HakoSharedMemoryShm::destroy_memory(int32_t key)
{
    SharedMemoryInfoType *info = find_memory_info(this->shared_memory_map_, key);
    if (info == nullptr) {
        return;
    }
    void *addr = info->addr;
    if (addr != nullptr) {
        (void)shmdt(addr);
        (void)shmctl (info->shm_id, IPC_RMID, 0);
        hako::utils::sem::destroy(info->sem_id);
        this->shared_memory_map_.erase(key);
    }
    return;
}
