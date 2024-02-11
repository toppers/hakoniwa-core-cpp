#include "utils/hako_share/hako_sem.hpp"
#include "hako_log.hpp"
//#include "utils/hako_logger.hpp"
#define HAKO_SEM_INX_MASTER   0
#define HAKO_SEM_INX_ASSETS  1

#ifdef MACOSX
#else
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};
#endif

int32_t hako::utils::sem::create(int32_t key)
{
    int32_t sem_id = semget(key, (1 + HAKO_DATA_MAX_ASSET_NUM), 0666 | IPC_CREAT);
    if (sem_id < 0) {
        HAKO_LOG_ERROR("semget() error: key=%d errno=%d", key, errno);
        return -1;
    }

    union semun argument;
    unsigned short values[1 + HAKO_DATA_MAX_ASSET_NUM];
    values[0] = 1;
    for (int i = 1; i <= HAKO_DATA_MAX_ASSET_NUM; i++) {
        values[i] = 0;
    }
    argument.array = values;
    int err = semctl(sem_id, 0, SETALL, argument);
    if (err < 0) {
        HAKO_LOG_ERROR("semctl() error: key=%d errno=%d", key, errno);
        hako::utils::sem::destroy(sem_id);
        return -1;
    }
    return sem_id;
}

void hako::utils::sem::destroy(int32_t sem_id)
{
    (void)semctl(sem_id, 1, IPC_RMID, NULL);
    return;
}

void hako::utils::sem::asset_down(int32_t sem_id, int32_t asset_id)
{
    struct sembuf sop;
    sop.sem_num =  HAKO_SEM_INX_ASSETS + asset_id;     // Semaphore number
    sop.sem_op  = -1;            // Semaphore operation is Lock
    sop.sem_flg =  0;            // Operation flag
    int32_t err = semop(sem_id, &sop, 1);
    if (err < 0) {
        HAKO_LOG_ERROR("asset_down: semop() error: sem_id=%d asset_id=%d errno=%d", sem_id, asset_id, errno);
    }
    return;
}
void hako::utils::sem::asset_up(int32_t sem_id, int32_t asset_id)
{
    struct sembuf sop;
    sop.sem_num =  HAKO_SEM_INX_ASSETS + asset_id;     // Semaphore number
    sop.sem_op  =  1;            // Semaphore operation is Lock
    sop.sem_flg =  0;            // Operation flag
    int32_t err = semop(sem_id, &sop, 1);
    if (err < 0) {
        HAKO_LOG_ERROR("asset_up: semop() error: sem_id=%d asset_id=%d errno=%d", sem_id, asset_id, errno);
    }
    return;
}
void hako::utils::sem::master_lock(int32_t sem_id)
{
    struct sembuf sop;
    sop.sem_num =  HAKO_SEM_INX_MASTER;     // Semaphore number
    sop.sem_op  = -1;            // Semaphore operation is Lock
    sop.sem_flg =  0;            // Operation flag
    int32_t err = semop(sem_id, &sop, 1);
    if (err < 0) {
        HAKO_LOG_ERROR("master_lock: semop() error: sem_id=%d errno=%d", sem_id, errno);
    }
    return;
}
void hako::utils::sem::master_unlock(int32_t sem_id)
{
    struct sembuf sop;
    sop.sem_num =  HAKO_SEM_INX_MASTER;     // Semaphore number
    sop.sem_op  =  1;            // Semaphore operation is Lock
    sop.sem_flg =  0;            // Operation flag
    int32_t err = semop(sem_id, &sop, 1);
    if (err < 0) {
        HAKO_LOG_ERROR("master_unlock: semop() error: sem_id=%d errno=%d", sem_id, errno);
    }
    return;
}
