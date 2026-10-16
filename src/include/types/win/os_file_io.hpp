#ifndef _WINDOWS_FILE_IO_HPP_
#define _WINDOWS_FILE_IO_HPP_

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

typedef struct {
    HANDLE handle;
    HANDLE map_handle;
    void *mmap_addr;
    size_t size;
    /* Exact byte range passed to the successful LockFileEx call. */
    size_t locked_size;
} WinHandleType;

extern int  win_open_rw(const char* filepath, WinHandleType *whp);
extern int  win_create_rw(const char* filepath, WinHandleType *whp);
/* Like win_create_rw, but fails instead of truncating when the file exists. */
extern int  win_create_new_rw(const char* filepath, WinHandleType *whp);
extern void *win_mmap(WinHandleType *whp);
extern void win_munmap(WinHandleType *whp);
extern void win_close(WinHandleType *whp);

extern int win_flock_acquire(WinHandleType *whp);
/* Returns 0 when acquired, 1 when busy, and -1 on another error. */
extern int win_flock_try_acquire(WinHandleType *whp);
/* Returns 0 on success and -1 when UnlockFileEx fails. */
extern int win_flock_release(WinHandleType *whp);
extern int win_pwrite(WinHandleType *whp, const void* buf, size_t count, off_t offset);
extern int win_pread(WinHandleType *whp, void* buf, size_t count, off_t offset);

static inline void win_filepath(const char* src, wchar_t *dst) 
{
    mbstowcs(dst, src, strlen(src) + 1);
}

#endif /* _WINDOWS_FILE_IO_HPP_ */
