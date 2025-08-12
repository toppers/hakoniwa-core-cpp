#include "utils/hako_share/impl/hako_mmap.hpp"

typedef struct {
    int fd;
    int size;
    HakoMmapObjectType mmap_obj;
} HakoMmapFileHandleObjectType;

HakoMmapObjectType* hako_mmap_create(std::string &filepath, int size)
{
    HakoMmapFileHandleObjectType *handle = (HakoMmapFileHandleObjectType *)malloc(sizeof(HakoMmapFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->fd = -1;
    handle->size = -1;
    handle->mmap_obj.mmap_addr = nullptr;
    handle->mmap_obj.obj = nullptr;
    struct stat stbuf;
    int ret = stat(filepath.c_str(), &stbuf);
    if ((ret < 0) || (stbuf.st_size < size)) {
        int fd = open(filepath.c_str(), O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            printf("ERROR: can not create mmap file:%s\n", filepath.c_str());
            return nullptr;
        }
        if (truncate(filepath.c_str(), size) != 0) {
            perror("ftruncate");
            close(fd);
            return nullptr;
        }
        close(fd);
        //printf("INFO: CREATED MMAP FILE: %s size=%d\n", filepath.c_str(), size);
    }
    handle->mmap_obj.obj = handle;
    handle->size = size;
    handle->fd = open(filepath.c_str(), O_RDWR);
    if (handle->fd < 0) {
        hako_mmap_destroy(&handle->mmap_obj);
        return nullptr;
    }
    handle->mmap_obj.mmap_addr = mmap(NULL, 
        handle->size, PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd, 0);
    if (handle->mmap_obj.mmap_addr == ((void*)-1)) {
        hako_mmap_destroy(&handle->mmap_obj);
        return nullptr;
    }
    return &handle->mmap_obj;
}

HakoMmapObjectType* hako_mmap_open(std::string &filepath)
{
    HakoMmapFileHandleObjectType *handle = (HakoMmapFileHandleObjectType *)malloc(sizeof(HakoMmapFileHandleObjectType));
    if (handle == nullptr) {
        return nullptr;
    }
    handle->fd = -1;
    handle->size = -1;
    handle->mmap_obj.mmap_addr = nullptr;
    handle->mmap_obj.obj = nullptr;
    struct stat stbuf;
    int ret = stat(filepath.c_str(), &stbuf);
    if (ret < 0) {
        return nullptr;
    }
    handle->mmap_obj.obj = handle;
    handle->size = stbuf.st_size;
    handle->fd = open(filepath.c_str(), O_RDWR);
    if (handle->fd < 0) {
        hako_mmap_close(&handle->mmap_obj);
        return nullptr;
    }
    handle->mmap_obj.mmap_addr = mmap(NULL, 
        handle->size, PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd, 0);
    if (handle->mmap_obj.mmap_addr == ((void*)-1)) {
        hako_mmap_close(&handle->mmap_obj);
        return nullptr;
    }
    return &handle->mmap_obj;
}

void hako_mmap_close(HakoMmapObjectType *handle)
{
    HakoMmapFileHandleObjectType *real;
    if (handle == nullptr) {
        return;
    }
    real = (HakoMmapFileHandleObjectType*)handle->obj;
    if (handle->mmap_addr != nullptr) {
        munmap(handle->mmap_addr, real->size);
    }
    if (real->fd >= 0) {
        close(real->fd);
    }
    free(real);
    return;
}

void hako_mmap_destroy(HakoMmapObjectType *handle)
{
    hako_mmap_close(handle);
    //TODO remove file
    return;
}