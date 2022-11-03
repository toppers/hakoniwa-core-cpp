#ifndef _WINDOWS_FILE_IO_HPP_
#define _WINDOWS_FILE_IO_HPP_

#include <windows.h>

typedef struct {
    HANDLE handle;
    HANDLE map_handle;
    void *mmap_addr;
    int size;
} WinHandleType;

extern int  win_open_rw(const char* filepath, WinHandleType *whp);
extern int  win_create_rw(const char* filepath, WinHandleType *whp);
extern void *win_mmap(WinHandleType *whp);
extern void win_munmap(WinHandleType *whp);
extern void win_close(WinHandleType *whp);

extern void win_flock_acquire(WinHandleType *whp);
extern void win_flock_release(WinHandleType *whp);
extern int win_pwrite(WinHandleType *whp, const void* buf, size_t count, off_t offset);
extern int win_pread(WinHandleType *whp, void* buf, size_t count, off_t offset);

static inline void win_filepath(const char* src, wchar_t *dst) 
{
    mbstowcs(dst, src, strlen(src));
}

#endif /* _WINDOWS_FILE_IO_HPP_ */