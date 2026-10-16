#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_share/impl/hako_flock_wait.hpp"

#include <cerrno>
#include <chrono>
#include <thread>

typedef struct {
    int fd;
    HakoFlockObjectType flock_obj;
} HakoFlockFileHandleObjectType;


HakoFlockObjectType* hako_flock_create(
    std::string &filepath, uint64_t create_size)
{
    HakoFlockFileHandleObjectType *handle = (HakoFlockFileHandleObjectType *)malloc(sizeof(HakoFlockFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->fd = -1;
    handle->flock_obj.lock_count = 0;
    handle->flock_obj.created = false;
    handle->flock_obj.obj = nullptr;
    int fd = open(filepath.c_str(), O_CREAT | O_EXCL | O_RDWR, 0644);
    if (fd >= 0) {
        handle->flock_obj.created = true;
        if (ftruncate(fd, static_cast<off_t>(create_size)) < 0) {
            printf("ERROR: can not set size of flock file:%s\n", filepath.c_str());
            perror("ftruncate");
            close(fd);
            return nullptr;
        }
        close(fd);
        printf("INFO: CREATED FLOCK FILE: %s\n", filepath.c_str());
    }
    else if (errno != EEXIST) {
        printf("ERROR: can not create flock file:%s\n", filepath.c_str());
        free(handle);
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

HakoFlockObjectType* hako_flock_open(std::string &filepath)
{
    HakoFlockFileHandleObjectType *handle = (HakoFlockFileHandleObjectType *)malloc(sizeof(HakoFlockFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->fd = -1;
    handle->flock_obj.lock_count = 0;
    handle->flock_obj.created = false;
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
        printf("ERROR: can not open flock file:%s\n", filepath.c_str());
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
        if (!hako_flock_wait_is_enabled()) {
            while (flock(real->fd, LOCK_EX) != 0) {
                if (errno == EINTR) {
                    continue;
                }
                throw HakoFlockWaitError(
                    HakoFlockWaitErrorType::SystemError,
                    "file-lock acquisition failed");
            }
        }
        else {
            const uint64_t started_at_ms = hako_flock_wait_now_ms();
            while (true) {
                hako_flock_wait_checkpoint(started_at_ms);
                if (flock(real->fd, LOCK_EX | LOCK_NB) == 0) {
                    break;
                }
                if ((errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EINTR)) {
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
    if (flock(real->fd, LOCK_EX | LOCK_NB) == 0) {
        handle->lock_count = 1;
        return 0;
    }
    if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
        return 1;
    }
    return -1;
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

bool hako_flock_write_bytes(
    HakoFlockObjectType *handle, uint64_t offset, const void *data, size_t size)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    return pwrite(real->fd, data, size, static_cast<off_t>(offset)) ==
           static_cast<ssize_t>(size);
}

bool hako_flock_read_bytes(
    HakoFlockObjectType *handle, uint64_t offset, void *data, size_t size)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    return pread(real->fd, data, size, static_cast<off_t>(offset)) ==
           static_cast<ssize_t>(size);
}

uint64_t hako_flock_size(HakoFlockObjectType *handle)
{
    HakoFlockFileHandleObjectType *real =
        (HakoFlockFileHandleObjectType*)handle->obj;
    struct stat st{};
    if (fstat(real->fd, &st) != 0 || st.st_size < 0) {
        return 0;
    }
    return static_cast<uint64_t>(st.st_size);
}
