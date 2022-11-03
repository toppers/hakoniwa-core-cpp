#include "utils/hako_share/impl/hako_mmap.hpp"
#include "types/win/os_file_io.hpp"

typedef struct {
    WinHandleType fd;
    int size;
    HakoMmapObjectType mmap_obj;
} HakoMmapFileHandleObjectType;

HakoMmapObjectType* hako_mmap_create(std::string &filepath, int size)
{
    HakoMmapFileHandleObjectType *handle = (HakoMmapFileHandleObjectType *)malloc(sizeof(HakoMmapFileHandleObjectType));
    if (handle == nullptr) {
        printf("ERROR: malloc() return error\n");
        return nullptr;
    }
    handle->size = -1;
    handle->mmap_obj.mmap_addr = nullptr;
    handle->mmap_obj.obj = nullptr;
    int ret = win_open_rw(filepath.c_str(), &handle->fd);
    if (ret < 0) {
        handle->fd.size = size;
        ret = win_create_rw(filepath.c_str(), &handle->fd);
        if (ret < 0) {
            printf("ERROR: can not create mmap file:%s\n", filepath.c_str());
            return nullptr;
        }
    }
    handle->mmap_obj.obj = handle;
    handle->size = size;
    handle->mmap_obj.mmap_addr = win_mmap(&handle->fd);
    if (handle->mmap_obj.mmap_addr == nullptr) {
        hako_mmap_destroy(&handle->mmap_obj);
        printf("ERROR: win_mmap() return error\n");
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
    handle->size = -1;
    handle->mmap_obj.mmap_addr = nullptr;
    handle->mmap_obj.obj = nullptr;
    int ret = win_open_rw(filepath.c_str(), &handle->fd);
    if (ret < 0) {
        return nullptr;
    }
    handle->mmap_obj.obj = handle;
    handle->size = handle->fd.size;
    handle->mmap_obj.mmap_addr = win_mmap(&handle->fd);
    if (handle->mmap_obj.mmap_addr == nullptr) {
        hako_mmap_destroy(&handle->mmap_obj);
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
        win_munmap(&real->fd);
    }
    if (real->fd.handle != INVALID_HANDLE_VALUE) {
        win_close(&real->fd);
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