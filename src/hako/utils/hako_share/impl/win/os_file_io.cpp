#include "types/hako_types.hpp"
#include "types/win/os_file_io.hpp"
#include "utils/hako_assert.hpp"
#include <stdio.h>
#include <limits>

static inline DWORD hako_win_low32(size_t value)
{
    return static_cast<DWORD>(static_cast<uint64_t>(value) & 0xFFFFFFFFULL);
}

static inline DWORD hako_win_high32(size_t value)
{
    return static_cast<DWORD>((static_cast<uint64_t>(value) >> 32) & 0xFFFFFFFFULL);
}

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
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(whp->handle, &file_size) == FALSE) {
        return -1;
    }
    if (file_size.QuadPart < 0) {
        return -1;
    }
    if (static_cast<unsigned long long>(file_size.QuadPart) >
        static_cast<unsigned long long>(std::numeric_limits<size_t>::max())) {
        return -1;
    }
    whp->size = static_cast<size_t>(file_size.QuadPart);
    //printf("INFO: win_open_rw() size=%zu\n", whp->size);
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
    printf("INFO: win_create_rw() want size=%zu\n", whp->size);

    if (whp->size > static_cast<size_t>(std::numeric_limits<LONGLONG>::max())) {
        return -1;
    }

    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(whp->size);    
    if (SetFilePointerEx(whp->handle, li, nullptr, FILE_BEGIN) == FALSE) {
        return -1;
    }
    if (SetEndOfFile(whp->handle) == FALSE) {
        return -1;
    }
    LARGE_INTEGER file_size;
    if (GetFileSizeEx(whp->handle, &file_size) == FALSE) {
        return -1;
    }
    if (file_size.QuadPart < 0) {
        return -1;
    }
    whp->size = static_cast<size_t>(file_size.QuadPart);
    //printf("INFO: win_create_rw() size=%zu\n", whp->size);
    return 0;
}

void* win_mmap(WinHandleType *whp)
{
    if (whp == NULL) {
        return NULL;
    }
    DWORD size_low = hako_win_low32(whp->size);
    DWORD size_high = hako_win_high32(whp->size);

    whp->map_handle = CreateFileMapping(
        whp->handle,
        0,
        PAGE_READWRITE,
        size_high,
        size_low,
        0
    );
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
    OVERLAPPED overlap;
    memset((void*)&overlap, 0, sizeof(OVERLAPPED));
    overlap.Offset = 0;
    overlap.OffsetHigh = 0;

    DWORD len_low = hako_win_low32(whp->size);
    DWORD len_high = hako_win_high32(whp->size);

    BOOL ret = LockFileEx(
        whp->handle,
        LOCKFILE_EXCLUSIVE_LOCK,
        0,
        len_low,
        len_high,
        &overlap
    );

    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_flock_acquire() error:%lu\n", err);
    }
}
void win_flock_release(WinHandleType *whp)
{
    OVERLAPPED overlap;
    memset((void*)&overlap, 0, sizeof(OVERLAPPED));
    overlap.Offset = 0;
    overlap.OffsetHigh = 0;

    DWORD len_low = hako_win_low32(whp->size);
    DWORD len_high = hako_win_high32(whp->size);

    BOOL ret = UnlockFileEx(
        whp->handle,
        0,
        len_low,
        len_high,
        &overlap
    );

    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_flock_release() error:%lu\n", err);
    }
}
int win_pwrite(WinHandleType *whp, const void* buf, size_t count, off_t offset)
{
    if (count > static_cast<size_t>(std::numeric_limits<DWORD>::max())) {
        return -1;
    }

    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(offset);

    if (SetFilePointerEx(whp->handle, li, nullptr, FILE_BEGIN) == FALSE) {
        DWORD err = GetLastError();
        printf("win_pwrite SetFilePointerEx error:%lu\n", err);
        return -(int)err;
    }

    DWORD written = 0;
    BOOL ret = WriteFile(
        whp->handle,
        buf,
        static_cast<DWORD>(count),
        &written,
        0
    );

    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_pwrite error:%lu\n", err);
        return -(int)err;
    }

    return 0;
}
int win_pread(WinHandleType *whp, void* buf, size_t count, off_t offset)
{
    if (count > static_cast<size_t>(std::numeric_limits<DWORD>::max())) {
        return -1;
    }

    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(offset);

    if (SetFilePointerEx(whp->handle, li, nullptr, FILE_BEGIN) == FALSE) {
        DWORD err = GetLastError();
        printf("win_pread SetFilePointerEx error:%lu\n", err);
        return -(int)err;
    }

    DWORD read_size = 0;
    BOOL ret = ReadFile(
        whp->handle,
        buf,
        static_cast<DWORD>(count),
        &read_size,
        0
    );

    if (ret == FALSE) {
        DWORD err = GetLastError();
        printf("win_pread error:%lu\n", err);
        return -(int)err;
    }

    return 0;
}
