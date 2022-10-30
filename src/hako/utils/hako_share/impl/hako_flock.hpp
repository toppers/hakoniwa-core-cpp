#ifndef _HAKO_FLOCK_HPP_
#define _HAKO_FLOCK_HPP_


#include "types/hako_types.hpp"
#include <string>

typedef struct {
    int lock_count;
    void *obj; /* do not use. for implementation. */
} HakoFlockObjectType;

extern HakoFlockObjectType* hako_flock_create(std::string &filepath);
extern HakoFlockObjectType* hako_flock_open(std::string &filepath);
extern void hako_flock_acquire(HakoFlockObjectType *handle);
extern void hako_flock_write(HakoFlockObjectType *handle, int index, int value);
extern void hako_flock_read(HakoFlockObjectType *handle, int index, int *rvalue);
extern void hako_flock_release(HakoFlockObjectType *handle);
extern void hako_flock_close(HakoFlockObjectType *handle);
extern void hako_flock_destroy(HakoFlockObjectType *handle);

#endif /* _HAKO_FLOCK_HPP_ */