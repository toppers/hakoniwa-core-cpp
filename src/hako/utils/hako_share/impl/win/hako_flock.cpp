#include "utils/hako_share/impl/hako_flock.hpp"

typedef struct {
    int fd;
    HakoFlockObjectType flock_obj;
} HakoFlockFileHandleObjectType;


HakoFlockObjectType* hako_flock_create(std::string &filepath)
{
    return nullptr;
}

HakoFlockObjectType* hako_flock_open(std::string &filepath)
{
    return nullptr;
}

void hako_flock_close(HakoFlockObjectType *handle)
{

    return;
}

void hako_flock_destroy(HakoFlockObjectType *handle)
{
    hako_flock_close(handle);
    return;
}

void hako_flock_acquire(HakoFlockObjectType *handle)
{
    return;
}

void hako_flock_release(HakoFlockObjectType *handle)
{
    return;
}

void hako_flock_write(HakoFlockObjectType *handle, int index, int value)
{
}

void hako_flock_read(HakoFlockObjectType *handle, int index, int *rvalue)
{
}
