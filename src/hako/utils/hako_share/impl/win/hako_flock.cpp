#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_share/impl/hako_flock_wait.hpp"
#include "types/win/os_file_io.hpp"

#include <chrono>
#include <thread>

typedef struct {
    WinHandleType fd;
    HakoFlockObjectType flock_obj;
} HakoFlockFileHandleObjectType;


HakoFlockObjectType* hako_flock_create(std::string &filepath)
{
    HakoFlockFileHandleObjectType *handle = (HakoFlockFileHandleObjectType *)malloc(sizeof(HakoFlockFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->flock_obj.lock_count = 0;
    handle->flock_obj.obj = nullptr;
    int ret = win_open_rw(filepath.c_str(), &handle->fd);
    if (ret < 0) {
        handle->fd.size = sizeof(int) * HAKO_CONFIG_MAX_SEM;
        ret = win_create_rw(filepath.c_str(), &handle->fd);
        if (ret < 0) {
            printf("ERROR: can not create flock file:%s\n", filepath.c_str());
            return nullptr;
        }
        printf("INFO: CREATED FLOCK FILE: %s\n", filepath.c_str());
    }
    handle->flock_obj.obj = handle;
    return &handle->flock_obj;
}

HakoFlockObjectType* hako_flock_open(std::string &filepath)
{
    HakoFlockFileHandleObjectType *handle = (HakoFlockFileHandleObjectType *)malloc(sizeof(HakoFlockFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->flock_obj.lock_count = 0;
    handle->flock_obj.obj = nullptr;
    int ret = win_open_rw(filepath.c_str(), &handle->fd);
    if (ret < 0) {
        free(handle);
        printf("ERROR: can not create flock file:%s\n", filepath.c_str());
        return nullptr;
    }
    handle->flock_obj.obj = handle;
    return &handle->flock_obj;
}

void hako_flock_close(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real;
    if (handle == nullptr) {
        return;
    }
    real = (HakoFlockFileHandleObjectType*)handle->obj;
    if (real->fd.handle != INVALID_HANDLE_VALUE) {
        win_close(&real->fd);
    }
    free(real);
    return;
}

void hako_flock_destroy(HakoFlockObjectType *handle)
{
    hako_flock_close(handle);
    return;
}

void hako_flock_acquire(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;
    if (handle->lock_count <= 0) {
        if (!hako_flock_wait_is_enabled()) {
            (void)win_flock_acquire(&real->fd);
        }
        else {
            const uint64_t started_at_ms = hako_flock_wait_now_ms();
            while (true) {
                hako_flock_wait_checkpoint(started_at_ms);
                int ret = win_flock_try_acquire(&real->fd);
                if (ret == 0) {
                    break;
                }
                if (ret < 0) {
                    throw HakoFlockWaitError(
                        HakoFlockWaitErrorType::SystemError,
                        "file-lock acquisition failed");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
        }
        handle->lock_count = 1;
    }
    else {
        handle->lock_count++;
    }
    return;
}

void hako_flock_release(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;
    if (handle->lock_count > 0) {
        handle->lock_count--;
    }

    if (handle->lock_count <= 0) {
        (void)win_flock_release(&real->fd);
    }
    return;
}

void hako_flock_write(HakoFlockObjectType *handle, int index, int value)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;
    ssize_t ret = win_pwrite(&real->fd, (const void *)&value, sizeof(value), (index * sizeof(value)));
    if (ret < 0) {
        printf("ERROR: hako_flock_write() returns error=%lld\n", ret);
    }
}

void hako_flock_read(HakoFlockObjectType *handle, int index, int *rvalue)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;
    ssize_t ret = win_pread(&real->fd, (void *)rvalue, sizeof(int), (index * sizeof(int)));
    if (ret < 0) {
        printf("ERROR: hako_flock_read() returns error=%lld\n", ret);
    }
}
