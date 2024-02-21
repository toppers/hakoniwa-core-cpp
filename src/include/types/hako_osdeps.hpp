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
#include <sys/file.h>
typedef pid_t pid_type;
#else

#include "windows.h"
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
typedef DWORD pid_type;
static inline void usleep(long microseconds) {
    Sleep(microseconds / 1000);
}
#endif /* OS_TYPE */

#endif /* _HAKO_OSDEPS_HPP_ */