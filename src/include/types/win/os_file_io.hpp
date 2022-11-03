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

static inline void win_filepath(const char* src, wchar_t *dst) 
{
    mbstowcs(dst, src, strlen(src));
}

#endif /* _WINDOWS_FILE_IO_HPP_ */