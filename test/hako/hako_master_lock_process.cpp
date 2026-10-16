#include "utils/hako_share/hako_master_lock.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"
#include "utils/hako_share/impl/hako_flock.hpp"
#include "utils/hako_share/impl/hako_flock_wait.hpp"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifndef WIN32
#include <unistd.h>
#endif

namespace {

void print_inspection(bool force_wait = false)
{
    using namespace hako::utils::sem::flock;
    const MasterLockInspection value = inspect_master_lock(500, force_wait);
    std::cout << master_lock_health_name(value.health) << " "
              << master_lock_state_name(value.state) << " "
              << value.counter << " " << value.owner_pid << " "
              << value.generation << " " << value.reason << " cause="
              << master_lock_unknown_cause_name(value.unknown_cause)
              << std::endl;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    const std::string mode = argv[1];
    constexpr int32_t sem_id = 0x00FF;
    if (mode == "inspect") {
        print_inspection();
        return 0;
    }
    if (mode == "recreate-inspect") {
        if (hako::utils::sem::flock::create(sem_id) < 0) {
            return 3;
        }
        print_inspection(true);
        return 0;
    }
    if (mode == "legacy-hold") {
        const char *configured_path = std::getenv("HAKO_TEST_FLOCK_PATH");
        if (configured_path == nullptr) {
            return 10;
        }
        std::string path = configured_path;
        HakoFlockObjectType *handle = hako_flock_open(path);
        if (handle == nullptr) {
            return 9;
        }
        hako_flock_acquire(handle);
        std::cout << "READY" << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
#ifndef WIN32
    if (mode == "zombie-owner") {
        int ready_pipe[2]{};
        if (pipe(ready_pipe) != 0) {
            return 4;
        }
        const pid_t child = fork();
        if (child < 0) {
            return 5;
        }
        if (child == 0) {
            close(ready_pipe[0]);
            if (hako::utils::sem::flock::create(sem_id) < 0) {
                std::_Exit(6);
            }
            hako::utils::sem::flock::master_lock(sem_id);
            const char ready = 'R';
            (void)write(ready_pipe[1], &ready, sizeof(ready));
            close(ready_pipe[1]);
            std::_Exit(0);
        }
        close(ready_pipe[1]);
        char ready = '\0';
        if (read(ready_pipe[0], &ready, sizeof(ready)) != sizeof(ready)) {
            return 7;
        }
        close(ready_pipe[0]);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "READY" << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
#endif
    if (mode == "try-lock" || mode == "try-lock-bounded") {
        if (hako::utils::sem::flock::create(sem_id) < 0) {
            return 3;
        }
        volatile sig_atomic_t interrupted = 0;
        std::unique_ptr<HakoFlockWaitScope> wait_scope;
        if (mode == "try-lock-bounded") {
            wait_scope = std::make_unique<HakoFlockWaitScope>(
                HakoFlockWaitOptions{80, &interrupted});
        }
        try {
            hako::utils::sem::flock::master_lock(sem_id);
        }
        catch (const HakoFlockWaitError& error) {
            const auto inspection =
                hako::utils::sem::flock::inspect_master_lock(0, false);
            std::cout << "WAIT_STOP "
                      << hako::utils::sem::flock::master_lock_unknown_cause_name(
                             inspection.unknown_cause)
                      << " " << error.what() << std::endl;
            return 0;
        }
        catch (const hako::utils::sem::flock::MasterLockStateError& error) {
            std::cout << "FAIL_STOP "
                      << hako::utils::sem::flock::master_lock_health_name(
                             error.health())
                      << " " << error.what() << " cause="
                      << hako::utils::sem::flock::master_lock_unknown_cause_name(
                             error.unknown_cause())
                      << std::endl;
            return 0;
        }
        return 8;
    }
    if (mode == "lock-unlock") {
        if (hako::utils::sem::flock::create(sem_id) < 0) {
            return 3;
        }
        hako::utils::sem::flock::master_lock(sem_id);
        hako::utils::sem::flock::master_unlock(sem_id);
        std::cout << "ROUNDTRIP_OK" << std::endl;
        return 0;
    }
    if (hako::utils::sem::flock::create(sem_id) < 0) {
        return 3;
    }
    hako::utils::sem::flock::master_lock(sem_id);
    if (mode == "crash") {
        std::_Exit(0);
    }
    if (mode == "hold") {
        std::cout << "READY" << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return 2;
}
