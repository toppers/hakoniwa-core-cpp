#include "utils/hako_share/impl/hako_process_identity.hpp"

#include <cerrno>
#include <fstream>
#include <sstream>
#include <string>

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <signal.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

namespace {

bool process_start_token(int64_t pid, uint64_t *token)
{
    if (pid <= 0 || token == nullptr) {
        return false;
    }
#ifdef WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                 static_cast<DWORD>(pid));
    if (process == nullptr) {
        return false;
    }
    FILETIME creation{}, exit{}, kernel{}, user{};
    const bool ok = GetProcessTimes(process, &creation, &exit, &kernel, &user) != FALSE;
    CloseHandle(process);
    if (!ok) {
        return false;
    }
    *token = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) |
             static_cast<uint64_t>(creation.dwLowDateTime);
    return true;
#elif defined(__APPLE__)
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, static_cast<int>(pid)};
    struct kinfo_proc info{};
    size_t size = sizeof(info);
    if (sysctl(mib, 4, &info, &size, nullptr, 0) != 0 || size == 0) {
        return false;
    }
    *token = static_cast<uint64_t>(info.kp_proc.p_starttime.tv_sec) * 1000000ULL +
             static_cast<uint64_t>(info.kp_proc.p_starttime.tv_usec);
    return true;
#else
    std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!std::getline(stat, line)) {
        return false;
    }
    const auto end_comm = line.rfind(')');
    if (end_comm == std::string::npos || end_comm + 2 >= line.size()) {
        return false;
    }
    std::istringstream fields(line.substr(end_comm + 2));
    std::string value;
    for (int field = 3; field <= 22; ++field) {
        if (!(fields >> value)) {
            return false;
        }
    }
    try {
        *token = std::stoull(value);
    }
    catch (...) {
        return false;
    }
    return true;
#endif
}

} // namespace

bool hako_current_process_identity(HakoProcessIdentity *identity)
{
    if (identity == nullptr) {
        return false;
    }
#ifdef WIN32
    identity->pid = static_cast<int64_t>(GetCurrentProcessId());
#else
    identity->pid = static_cast<int64_t>(getpid());
#endif
    return process_start_token(identity->pid, &identity->start_token);
}

HakoProcessIdentityStatus hako_check_process_identity(
    const HakoProcessIdentity& identity)
{
    if (identity.pid <= 0 || identity.start_token == 0) {
        return HakoProcessIdentityStatus::Unknown;
    }
#ifdef WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                 static_cast<DWORD>(identity.pid));
    if (process == nullptr) {
        const DWORD error = GetLastError();
        return (error == ERROR_INVALID_PARAMETER)
            ? HakoProcessIdentityStatus::Dead
            : HakoProcessIdentityStatus::Unknown;
    }
    DWORD exit_code = 0;
    const bool alive = GetExitCodeProcess(process, &exit_code) && exit_code == STILL_ACTIVE;
    CloseHandle(process);
    if (!alive) {
        return HakoProcessIdentityStatus::Dead;
    }
#else
    if (kill(static_cast<pid_t>(identity.pid), 0) != 0) {
        if (errno == ESRCH) {
            return HakoProcessIdentityStatus::Dead;
        }
        if (errno != EPERM) {
            return HakoProcessIdentityStatus::Unknown;
        }
    }
#endif
    uint64_t actual = 0;
    if (!process_start_token(identity.pid, &actual)) {
        return HakoProcessIdentityStatus::Unknown;
    }
    return actual == identity.start_token
        ? HakoProcessIdentityStatus::Alive
        : HakoProcessIdentityStatus::Dead;
}
