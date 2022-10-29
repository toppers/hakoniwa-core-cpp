#include "types/hako_types.hpp"

#include "utils/hako_share/hako_shared_memory.hpp"
#include "utils/hako_share/hako_sem.hpp"

int32_t hako::utils::HakoSharedMemory::create_memory(int32_t key, int32_t size)
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
    this->shared_memory_map_.insert(std::make_pair(key, info));
    return shmid;
}
void* hako::utils::HakoSharedMemory::load_memory(int32_t key, int32_t size)
{
    int32_t total_size = size + sizeof(SharedMemoryMetaDataType);
    int32_t shmid = shmget(key, total_size, S_IRUSR | S_IWUSR);
    if (shmid < 0) {
        printf("ERROR: shmget() key=%d size=%d error=%d\n", key, size, errno);
        return nullptr;
    }
    return this->load_memory_shmid(key, shmid);
}

void* hako::utils::HakoSharedMemory::load_memory_shmid(int32_t key, int32_t shmid)
{
    void *shared_memory = shmat(shmid, 0, 0);
    if (shared_memory == ((void*)-1)) {
        return nullptr;
    }
    SharedMemoryMetaDataType *metap = static_cast<SharedMemoryMetaDataType*>(shared_memory);
    SharedMemoryInfoType info;
    info.addr = metap;
    info.shm_id = metap->shm_id;
    info.sem_id = metap->sem_id;
    this->shared_memory_map_.insert(std::make_pair(key, info));
    return &this->shared_memory_map_[key].addr->data[0];
}

void* hako::utils::HakoSharedMemory::lock_memory(int32_t key)
{
#if 1
    hako::utils::sem::master_lock(this->shared_memory_map_[key].sem_id);
#else
    struct sembuf sop;
    sop.sem_num =  0;            // Semaphore number
    sop.sem_op  = -1;            // Semaphore operation is Lock
    sop.sem_flg =  0;            // Operation flag
    int32_t err = semop(this->shared_memory_map_[key].sem_id, &sop, 1);
    if (err < 0) {
        printf("ERROR: unlock_memory() semop() error=%d key=%d\n", errno, key);
    }
#endif
    return &this->shared_memory_map_[key].addr->data[0];
}

void hako::utils::HakoSharedMemory::unlock_memory(int32_t key)
{
#if 1
    hako::utils::sem::master_unlock(this->shared_memory_map_[key].sem_id);
#else
    struct sembuf sop;
    sop.sem_num =  0;            // Semaphore number
    sop.sem_op  =  1;            // Semaphore operation is Lock
    sop.sem_flg =  0;            // Operation flag
    int32_t err = semop(this->shared_memory_map_[key].sem_id, &sop, 1);
    if (err < 0) {
        printf("ERROR: unlock_memory() semop() error=%d key=%d\n", errno, key);
    }
#endif
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
        (void)shmdt(addr);
        (void)shmctl (this->shared_memory_map_[key].shm_id, IPC_RMID, 0);
#if 1
        hako::utils::sem::destroy(this->shared_memory_map_[key].sem_id);
#else
        (void)semctl(this->shared_memory_map_[key].sem_id, 1, IPC_RMID, NULL);
#endif
        this->shared_memory_map_.erase(key);
    }
    return;
}