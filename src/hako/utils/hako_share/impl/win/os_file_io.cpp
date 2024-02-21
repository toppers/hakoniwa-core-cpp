#include "types/hako_types.hpp"
#include "types/win/os_file_io.hpp"
#include "utils/hako_assert.hpp"
#include <stdio.h>

int  win_open_rw(const char* filepath, WinHandleType *whp)
{
    wchar_t wfile[4096];
    win_filepath(filepath, wfile);
    whp->handle = CreateFileW(
                    wfile, //lpFileName
                    GENERIC_READ | GENERIC_WRITE, //dwDesiredAccess
                    FILE_SHARE_READ| FILE_SHARE_WRITE, //dwShareMode
                    0, //lpSecurityAttributes
                    OPEN_EXISTING, //dwCreationDisposition
                    FILE_ATTRIBUTE_NORMAL, //dwFlagsAndAttributes
                    0//hTemplateFile
                    );
    if (whp->handle == INVALID_HANDLE_VALUE) {
        //printf("ERROR: win_open_rw(): CreateFileW() error\n");
        return -1;
    }
    whp->size = GetFileSize(whp->handle, 0);
    //printf("INFO: win_open_rw() size=%d\n", whp->size);
    return 0;
}

int  win_create_rw(const char* filepath, WinHandleType *whp)
{
    wchar_t wfile[4096];
    win_filepath(filepath, wfile);
    whp->handle = CreateFileW(
                    wfile, //lpFileName
                    GENERIC_READ | GENERIC_WRITE, //dwDesiredAccess
                    FILE_SHARE_READ| FILE_SHARE_WRITE, //dwShareMode
                    0, //lpSecurityAttributes
                    CREATE_ALWAYS, //dwCreationDisposition
                    FILE_ATTRIBUTE_NORMAL, //dwFlagsAndAttributes
                    0//hTemplateFile
                    );
    if (whp->handle == INVALID_HANDLE_VALUE) {
        printf("ERROR: win_create_rw(): CreateFileW() error\n");
        return -1;
    }
    printf("INFO: win_create_rw() want size=%d\n", whp->size);
    (void)SetFileValidData(whp->handle, whp->size);
    void *bufp = malloc(whp->size);
    HAKO_ASSERT(bufp != NULL);
    memset(bufp, 0, whp->size);
    win_pwrite(whp, bufp, whp->size, 0);
    free(bufp);
    whp->size = GetFileSize(whp->handle, 0);
    //printf("INFO: win_create_rw() size=%d\n", whp->size);
    return 0;
}

void* win_mmap(WinHandleType *whp)
{
    if (whp == NULL) {
        return NULL;
    }
    whp->map_handle = CreateFileMapping(whp->handle, 0, PAGE_READWRITE, 0, 0, 0);
    if (whp->map_handle == NULL) {
        return NULL;
    }
    whp->mmap_addr = MapViewOfFile(whp->map_handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (whp->mmap_addr == NULL) {
        return NULL;
    }
    return whp->mmap_addr;
}

void win_munmap(WinHandleType *whp)
{
    if (whp->map_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(whp->map_handle);
        whp->map_handle = INVALID_HANDLE_VALUE;
    }
    return;
}

void win_close(WinHandleType *whp)
{
    CloseHandle(whp->handle);
    return;
}

void win_flock_acquire(WinHandleType *whp)
{
    //BOOL ret = LockFile(whp->handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
    OVERLAPPED overlap;
    memset((void*)&overlap, 0, sizeof(LPOVERLAPPED));
    overlap.Offset = 0;
    overlap.OffsetHigh = 0;
    BOOL ret = LockFileEx(whp->handle, LOCKFILE_EXCLUSIVE_LOCK, 0, whp->size, 0, &overlap);
    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_flock_acquire() error:%ld\n", err);
    }
    return;
}

void win_flock_release(WinHandleType *whp)
{
    OVERLAPPED overlap;
    memset((void*)&overlap, 0, sizeof(LPOVERLAPPED));
    overlap.Offset = 0;
    overlap.OffsetHigh = 0;
    //BOOL ret = UnlockFile(whp->handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
    BOOL ret = UnlockFileEx(whp->handle, 0, whp->size, 0, &overlap);
    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_flock_release() error:%ld\n", err);
    }
    return;
}
int win_pwrite(WinHandleType *whp, const void* buf, size_t count, off_t offset)
{
    DWORD rsize = 0;
    (void)SetFilePointer(whp->handle, offset, 0, FILE_BEGIN);
    BOOL ret = WriteFile(whp->handle, buf, count, &rsize, 0);
    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_pwrite error:%ld\n", err);
        return - (int)err;
    }
    return 0;
}
int win_pread(WinHandleType *whp, void* buf, size_t count, off_t offset)
{
    DWORD rsize = 0;
    (void)SetFilePointer(whp->handle, offset, 0, FILE_BEGIN);
    BOOL ret = ReadFile(whp->handle, buf, count, &rsize, 0);
    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_pwrite error:%ld\n", err);
        return -(int)err;
    }
    return 0;
}
