#include "types/hako_types.hpp"
#include "types/win/os_file_io.hpp"

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
        return -1;
    }
    whp->size = GetFileSize(whp->handle, 0);
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
        return -1;
    }
    (void)SetFileValidData(whp->handle, whp->size);
    return 0;
}

void* win_mmap(WinHandleType *whp)
{
    whp->map_handle = CreateFileMapping(whp->handle, 0, PAGE_READWRITE, 0, 0, 0);
    whp->mmap_addr = MapViewOfFile(whp->map_handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
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
    LockFile(whp->handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
    return;
}

void win_flock_release(WinHandleType *whp)
{
    UnlockFile(whp->handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF);
    return;
}
int win_pwrite(WinHandleType *whp, const void* buf, size_t count, off_t offset)
{
    DWORD rsize = 0;
    (void)SetFilePointer(whp->handle, offset, 0, FILE_BEGIN);
    BOOL ret = WriteFile(whp->handle, buf, count, &rsize, 0);
    if (ret == FALSE) {
        return -1;
    }
    return 0;
}
int win_pread(WinHandleType *whp, void* buf, size_t count, off_t offset)
{
    DWORD rsize = 0;
    (void)SetFilePointer(whp->handle, offset, 0, FILE_BEGIN);
    BOOL ret = ReadFile(whp->handle, buf, count, &rsize, 0);
    if (ret == FALSE) {
        return -1;
    }
    return 0;
}
