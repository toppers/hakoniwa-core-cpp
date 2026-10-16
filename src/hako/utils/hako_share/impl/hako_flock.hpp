#ifndef _HAKO_FLOCK_HPP_
#define _HAKO_FLOCK_HPP_


#include "types/hako_types.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

typedef struct {
    int lock_count;
    bool created;
    void *obj; /* do not use. for implementation. */
} HakoFlockObjectType;

extern HakoFlockObjectType* hako_flock_create(
    std::string &filepath,
    uint64_t create_size = static_cast<uint64_t>(sizeof(int)) * HAKO_CONFIG_MAX_SEM);
extern HakoFlockObjectType* hako_flock_open(std::string &filepath);
extern void hako_flock_acquire(HakoFlockObjectType *handle);
/* Returns 0 when acquired, 1 when busy, and -1 on another error. */
extern int hako_flock_try_acquire(HakoFlockObjectType *handle);
extern void hako_flock_write(HakoFlockObjectType *handle, int index, int value);
extern void hako_flock_read(HakoFlockObjectType *handle, int index, int *rvalue);
extern bool hako_flock_write_bytes(
    HakoFlockObjectType *handle, uint64_t offset, const void *data, size_t size);
extern bool hako_flock_read_bytes(
    HakoFlockObjectType *handle, uint64_t offset, void *data, size_t size);
extern uint64_t hako_flock_size(HakoFlockObjectType *handle);
extern void hako_flock_release(HakoFlockObjectType *handle);
extern void hako_flock_close(HakoFlockObjectType *handle);
extern void hako_flock_destroy(HakoFlockObjectType *handle);

#endif /* _HAKO_FLOCK_HPP_ */
