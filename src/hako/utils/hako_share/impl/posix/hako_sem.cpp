#include "utils/hako_share/hako_sem.hpp"
#include "utils/hako_logger.hpp"
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
        hako::utils::logger::get("core")->error("semget() key={0} error={1}", key, errno);
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
        hako::utils::logger::get("core")->error("semctl() error = {0} sem_id={1}", errno, sem_id);
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
        hako::utils::logger::get("core")->error("asset_down() error = {0} sem_id={1} inx={2}", errno, sem_id, asset_id);
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
        hako::utils::logger::get("core")->error("asset_up() error = {0} sem_id={1} inx={2}", errno, sem_id, asset_id);
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
        hako::utils::logger::get("core")->error("master_lock() error = {0} sem_id={1} inx={2}", errno, sem_id, 0);
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
        hako::utils::logger::get("core")->error("master_unlock() error = {0} sem_id={1} inx={2}", errno, sem_id, 0);
    }
    return;
}
