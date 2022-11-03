#include "utils/hako_share/impl/hako_flock.hpp"
#include "types/win/os_file_io.hpp"

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
        (void)win_flock_acquire(&real->fd);
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
        printf("ERROR: hako_flock_write() error=%d\n", errno);
    }
}

void hako_flock_read(HakoFlockObjectType *handle, int index, int *rvalue)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;    
    ssize_t ret = win_pread(&real->fd, (void *)rvalue, sizeof(int), (index * sizeof(int)));
    if (ret < 0) {
        printf("ERROR: hako_flock_write() error=%d\n", errno);
    }
}
