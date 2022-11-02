#include "utils/hako_share/impl/hako_mmap.hpp"

typedef struct {
    int fd;
    int size;
    HakoMmapObjectType mmap_obj;
} HakoMmapFileHandleObjectType;

HakoMmapObjectType* hako_mmap_create(std::string &filepath, int size)
{
    return nullptr;
}

HakoMmapObjectType* hako_mmap_open(std::string &filepath)
{
    return nullptr;
}

void hako_mmap_close(HakoMmapObjectType *handle)
{
    return;
}

void hako_mmap_destroy(HakoMmapObjectType *handle)
{
    hako_mmap_close(handle);
    //TODO remove file
    return;
}