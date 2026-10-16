#include "utils/hako_share/hako_master_lock.hpp"
#include "utils/hako_share/hako_sem_flock.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

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
