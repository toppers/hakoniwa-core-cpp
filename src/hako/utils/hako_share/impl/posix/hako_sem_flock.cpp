#include "utils/hako_share/hako_sem.hpp"
#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_config_loader.hpp"
#include "utils/hako_logger.hpp"
#include "utils/hako_assert.hpp"
#define HAKO_SEM_INX_MASTER   0
#define HAKO_SEM_INX_ASSETS  1

static HakoFlockObjectType *flock_handle;
static void hako_sem_load(void)
{
    if (flock_handle != nullptr) {
        return;
    }
    char buf[4096];
    HakoConfigType config;
    hako_config_load(config);

    if (config.param == nullptr) {
        snprintf(buf, sizeof(buf), "./flock");
    }
    else {
        std::string core_mmap_path = config.param["core_mmap_path"];
        snprintf(buf, sizeof(buf), "%s/flock.bin", core_mmap_path.c_str());
    }
    //printf("INFO: flock path=%s\n", buf);
    std::string filepath(buf);
    flock_handle = hako_flock_open(filepath);
    HAKO_ASSERT(flock_handle != nullptr);
    return;
}
static int hako_sem_init(int index, int value)
{
    //pid_t pid = getpid();
    //printf("sem_init[%lld]: acquire\n", pid);
    hako_flock_acquire(flock_handle);
    {
        hako_flock_write(flock_handle, index, value);
    }
    hako_flock_release(flock_handle);
    //printf("sem_init[%lld]: release\n", pid);
    return 0;
}

static void hako_sem_down(int index)
{
    int value;
    //pid_t pid = getpid();
    hako_sem_load();
    //printf("sem_down[%d][%lld]: acquire\n", index, pid);
    hako_flock_acquire(flock_handle);
    while (true) {
        hako_flock_read(flock_handle, index, &value);
        if (value > 0) {
            hako_flock_write(flock_handle, index, --value);
            break;
        }
        hako_flock_release(flock_handle);
        //printf("sem_down[%d][%lld]: tmp release\n", index, pid);
#ifdef WIN32
        //https://learn.microsoft.com/ja-jp/windows/win32/api/synchapi/nf-synchapi-sleep
        Sleep(500);
#else
        usleep(500*1000);
#endif
        //printf("sem_down[%d][%lld]: tmp acquire\n", index, pid);
        hako_flock_acquire(flock_handle);
    }
    hako_flock_release(flock_handle);
    //printf("sem_down[%d][%lld]: release\n", index, pid);
    return;
}
static void hako_sem_up(int index)
{
    int value;
    //pid_t pid = getpid();
    hako_sem_load();
    //printf("sem_up[%d][%lld]: acquire\n", index, pid);
    hako_flock_acquire(flock_handle);
    {
        hako_flock_read(flock_handle, index, &value);
        hako_flock_write(flock_handle, index, ++value);
    }
    hako_flock_release(flock_handle);
    //printf("sem_up[%d][%lld]: release\n", index, pid);
    return;
}


int32_t hako::utils::sem::create(int32_t key)
{
    char buf[4096];
    HakoConfigType config;
    hako_config_load(config);

    if (config.param == nullptr) {
        snprintf(buf, sizeof(buf), "./flock");
    }
    else {
        std::string core_mmap_path = config.param["core_mmap_path"];
        snprintf(buf, sizeof(buf), "%s/flock.bin", core_mmap_path.c_str());
    }
    //printf("INFO: flock path=%s\n", buf);
    std::string filepath(buf);
    flock_handle = hako_flock_create(filepath);
    if (flock_handle == nullptr) {
        //hako::utils::logger::get("core")->error("hako_flock_create() key={0} error={1}", key, errno);
        return -1;
    }

    hako_sem_init(0, 1);
    for (int i = 1; i <= HAKO_DATA_MAX_ASSET_NUM; i++) {
        hako_sem_init(i, 0);
    }
    return key;
}

void hako::utils::sem::destroy(int32_t sem_id)
{
    HAKO_ASSERT(sem_id >= 0);
    hako_flock_destroy(flock_handle);
    return;
}

void hako::utils::sem::asset_down(int32_t sem_id, int32_t asset_id)
{
    HAKO_ASSERT(sem_id >= 0);
    hako_sem_down(HAKO_SEM_INX_ASSETS + asset_id);
    return;
}
void hako::utils::sem::asset_up(int32_t sem_id, int32_t asset_id)
{
    HAKO_ASSERT(sem_id >= 0);
    hako_sem_up(HAKO_SEM_INX_ASSETS + asset_id);
    return;
}
void hako::utils::sem::master_lock(int32_t sem_id)
{
    HAKO_ASSERT(sem_id >= 0);
    hako_sem_down(HAKO_SEM_INX_MASTER);
    return;
}
void hako::utils::sem::master_unlock(int32_t sem_id)
{
    HAKO_ASSERT(sem_id >= 0);
    hako_sem_up(HAKO_SEM_INX_MASTER);
    return;
}
