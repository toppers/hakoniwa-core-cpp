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
    handle->flock_obj.created = false;
    handle->flock_obj.obj = nullptr;
    handle->fd.size = sizeof(int) * HAKO_CONFIG_MAX_SEM;
    int ret = win_create_new_rw(filepath.c_str(), &handle->fd);
    if (ret == 0) {
        handle->flock_obj.created = true;
        printf("INFO: CREATED FLOCK FILE: %s\n", filepath.c_str());
    }
    else if (ret == 1) {
        ret = win_open_rw(filepath.c_str(), &handle->fd);
    }
    if (ret < 0) {
        printf("ERROR: can not create flock file:%s\n", filepath.c_str());
        free(handle);
        return nullptr;
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
    handle->flock_obj.created = false;
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
            if (win_flock_acquire(&real->fd) != 0) {
                throw HakoFlockWaitError(
                    HakoFlockWaitErrorType::SystemError,
                    "file-lock acquisition failed");
            }
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

int hako_flock_try_acquire(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    if (handle->lock_count > 0) {
        handle->lock_count++;
        return 0;
    }
    const int result = win_flock_try_acquire(&real->fd);
    if (result == 0) {
        handle->lock_count = 1;
    }
    return result;
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

bool hako_flock_write_bytes(
    HakoFlockObjectType *handle, uint64_t offset, const void *data, size_t size)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    const int result = win_pwrite(
        &real->fd, data, size, static_cast<off_t>(offset));
    if (result == 0) {
        const uint64_t end = offset + size;
        if (end > real->fd.size) {
            real->fd.size = static_cast<size_t>(end);
        }
        return true;
    }
    return false;
}

bool hako_flock_read_bytes(
    HakoFlockObjectType *handle, uint64_t offset, void *data, size_t size)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    if ((offset > real->fd.size) || (size > real->fd.size - offset)) {
        return false;
    }
    return win_pread(&real->fd, data, size, static_cast<off_t>(offset)) == 0;
}

uint64_t hako_flock_size(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    LARGE_INTEGER file_size{};
    if (GetFileSizeEx(real->fd.handle, &file_size) == FALSE ||
        file_size.QuadPart < 0) {
        return 0;
    }
    real->fd.size = static_cast<size_t>(file_size.QuadPart);
    return static_cast<uint64_t>(file_size.QuadPart);
}
