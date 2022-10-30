#ifndef _HAKO_OSDEPS_HPP_
#define _HAKO_OSDEPS_HPP_

#ifndef WIN32

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <errno.h>
#include <unistd.h>

#include <sys/shm.h>
#include<sys/sem.h>
#include <time.h>

#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>

#else

#endif /* OS_TYPE */

#endif /* _HAKO_OSDEPS_HPP_ */