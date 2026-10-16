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
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

namespace {

bool process_start_token(int64_t pid, uint64_t *token, bool *terminated = nullptr)
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
    if (!ok) {
        CloseHandle(process);
        return false;
    }
    *token = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) |
             static_cast<uint64_t>(creation.dwLowDateTime);
    if (terminated != nullptr) {
        DWORD exit_code = 0;
        if (!GetExitCodeProcess(process, &exit_code)) {
            CloseHandle(process);
            return false;
        }
        *terminated = exit_code != STILL_ACTIVE;
    }
    CloseHandle(process);
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
    if (terminated != nullptr) {
        *terminated = info.kp_proc.p_stat == SZOMB;
    }
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
    char state = '\0';
    for (int field = 3; field <= 22; ++field) {
        if (!(fields >> value)) {
            return false;
        }
        if (field == 3) {
            if (value.size() != 1) {
                return false;
            }
            state = value[0];
        }
    }
    try {
        *token = std::stoull(value);
    }
    catch (...) {
        return false;
    }
    if (terminated != nullptr) {
        *terminated = state == 'Z' || state == 'X' || state == 'x';
    }
    return true;
#endif
}

} // namespace

HakoProcessIdentityStatus
hako::utils::process_identity::detail::classify_lookup(
    bool definitely_missing,
    bool lookup_succeeded,
    bool terminated,
    uint64_t expected_start_token,
    uint64_t actual_start_token)
{
    if (definitely_missing || (lookup_succeeded && terminated)) {
        return HakoProcessIdentityStatus::Dead;
    }
    if (!lookup_succeeded || expected_start_token == 0 ||
        actual_start_token == 0) {
        return HakoProcessIdentityStatus::Unknown;
    }
    return actual_start_token == expected_start_token
        ? HakoProcessIdentityStatus::Alive
        : HakoProcessIdentityStatus::Dead;
}

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
        return hako::utils::process_identity::detail::classify_lookup(
            error == ERROR_INVALID_PARAMETER, false, false,
            identity.start_token, 0);
    }
    DWORD exit_code = 0;
    const bool exit_code_available =
        GetExitCodeProcess(process, &exit_code) != FALSE;
    CloseHandle(process);
    if (!exit_code_available) {
        return hako::utils::process_identity::detail::classify_lookup(
            false, false, false, identity.start_token, 0);
    }
    if (exit_code != STILL_ACTIVE) {
        return hako::utils::process_identity::detail::classify_lookup(
            false, true, true, identity.start_token, 0);
    }
#else
    if (kill(static_cast<pid_t>(identity.pid), 0) != 0) {
        if (errno == ESRCH) {
            return hako::utils::process_identity::detail::classify_lookup(
                true, false, false, identity.start_token, 0);
        }
        if (errno != EPERM) {
            return HakoProcessIdentityStatus::Unknown;
        }
    }
#endif
    uint64_t actual = 0;
    bool terminated = false;
    if (!process_start_token(identity.pid, &actual, &terminated)) {
        return hako::utils::process_identity::detail::classify_lookup(
            false, false, false, identity.start_token, 0);
    }
    return hako::utils::process_identity::detail::classify_lookup(
        false, true, terminated, identity.start_token, actual);
}
