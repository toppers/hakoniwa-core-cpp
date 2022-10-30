#ifndef _HAKO_MMAP_HPP_
#define _HAKO_MMAP_HPP_

#include "types/hako_types.hpp"
#include <string>

typedef struct {
    void *mmap_addr; /* shared memory */
    void *obj; /* do not use. for implementation. */
} HakoMmapObjectType;

extern HakoMmapObjectType* hako_mmap_create(std::string &filepath, int size);
extern HakoMmapObjectType* hako_mmap_open(std::string &filepath);
extern void hako_mmap_close(HakoMmapObjectType *handle);
extern void hako_mmap_destroy(HakoMmapObjectType *handle);

#endif /* _HAKO_MMAP_HPP_ */