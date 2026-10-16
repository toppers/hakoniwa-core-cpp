#include "utils/hako_share/hako_master_lock.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#ifndef WIN32
#include <unistd.h>
#endif

namespace {

void print_inspection()
{
    using namespace hako::utils::sem::flock;
    const MasterLockInspection value = inspect_master_lock(500, false);
    std::cout << master_lock_health_name(value.health) << " "
              << master_lock_state_name(value.state) << " "
              << value.counter << " " << value.owner_pid << " "
              << value.generation << " " << value.reason << std::endl;
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
        print_inspection();
        return 0;
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
    if (mode == "try-lock") {
        if (hako::utils::sem::flock::create(sem_id) < 0) {
            return 3;
        }
        try {
            hako::utils::sem::flock::master_lock(sem_id);
        }
        catch (const hako::utils::sem::flock::MasterLockStateError& error) {
            std::cout << "FAIL_STOP "
                      << hako::utils::sem::flock::master_lock_health_name(
                             error.health())
                      << " " << error.what() << std::endl;
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
