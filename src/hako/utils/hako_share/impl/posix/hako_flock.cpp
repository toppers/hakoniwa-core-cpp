#include "utils/hako_share/impl/hako_flock.hpp"

typedef struct {
    int fd;
    HakoFlockObjectType flock_obj;
} HakoFlockFileHandleObjectType;


HakoFlockObjectType* hako_flock_create(std::string &filepath)
{
    HakoFlockFileHandleObjectType *handle = (HakoFlockFileHandleObjectType *)malloc(sizeof(HakoFlockFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->fd = -1;
    handle->flock_obj.lock_count = 0;
    handle->flock_obj.obj = nullptr;
    struct stat stbuf;
    int ret = stat(filepath.c_str(), &stbuf);
    if (ret < 0) {
        int fd = open(filepath.c_str(), O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            printf("ERROR: can not create flock file:%s\n", filepath.c_str());
            return nullptr;
        }
        ftruncate(fd, sizeof(int) * HAKO_CONFIG_MAX_SEM);
        close(fd);
        printf("INFO: CREATED FLOCK FILE: %s\n", filepath.c_str());
    }
    handle->flock_obj.obj = handle;
    handle->fd = open(filepath.c_str(), O_RDWR);
    if (handle->fd < 0) {
        hako_flock_destroy(&handle->flock_obj);
        return nullptr;
    }
    return &handle->flock_obj;
}

HakoFlockObjectType* hako_flock_open(std::string &filepath)
{
    HakoFlockFileHandleObjectType *handle = (HakoFlockFileHandleObjectType *)malloc(sizeof(HakoFlockFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->fd = -1;
    handle->flock_obj.lock_count = 0;
    handle->flock_obj.obj = nullptr;
    struct stat stbuf;
    int ret = stat(filepath.c_str(), &stbuf);
    if (ret < 0) {
        free(handle);
        printf("ERROR: can not create flock file:%s\n", filepath.c_str());
        return nullptr;
    }
    handle->flock_obj.obj = handle;
    handle->fd = open(filepath.c_str(), O_RDWR);
    if (handle->fd < 0) {
        hako_flock_destroy(&handle->flock_obj);
        return nullptr;
    }
    return &handle->flock_obj;
}

void hako_flock_close(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real;
    if (handle == nullptr) {
        return;
    }
    real = (HakoFlockFileHandleObjectType*)handle->obj;
    if (real->fd >= 0) {
        close(real->fd);
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
        (void)flock(real->fd, LOCK_EX);
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
        (void)flock(real->fd, LOCK_UN);
    }
    return;
}

void hako_flock_write(HakoFlockObjectType *handle, int index, int value)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;    
    ssize_t ret = pwrite(real->fd, (const void *)&value, sizeof(value), (index * sizeof(value)));
    if (ret < 0) {
        printf("ERROR: hako_flock_write() error=%d\n", errno);
    }
}

void hako_flock_read(HakoFlockObjectType *handle, int index, int *rvalue)
{
    HakoFlockFileHandleObjectType *real;
    real = (HakoFlockFileHandleObjectType*)handle->obj;    
    ssize_t ret = pread(real->fd, (void *)rvalue, sizeof(int), (index * sizeof(int)));
    if (ret < 0) {
        printf("ERROR: hako_flock_read() error=%d\n", errno);
    }
}
