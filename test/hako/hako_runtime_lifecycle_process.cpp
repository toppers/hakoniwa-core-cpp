#include "utils/hako_share/hako_runtime_lifecycle.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

bool wait_for(const std::filesystem::path& path, int seconds = 10)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        if (std::filesystem::exists(path)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

void ready(const std::filesystem::path& path)
{
    std::ofstream(path) << "ready\n";
}

} // namespace

int main(int argc, char **argv)
{
    using namespace hako::utils::lifecycle;
    if (argc < 3) {
        return 2;
    }
    const std::string mode = argv[1];
    const std::string root = argv[2];
    try {
        if (mode == "recover") {
            RuntimeRecoveryGuard recovery;
            std::string reason;
            const auto health = recovery.acquire(root, &reason);
            std::cout << runtime_lifecycle_health_name(health) << " " << reason
                      << std::endl;
            return health == RuntimeLifecycleHealth::Ok ? 0 :
                   health == RuntimeLifecycleHealth::Busy ? 3 : 4;
        }
        if (argc < 5) {
            return 2;
        }
        const std::filesystem::path ready_path = argv[3];
        const std::filesystem::path release_path = argv[4];
        if (mode == "participant-create" || mode == "participant-attach") {
            RuntimeParticipantLease participant;
            participant.join(root, mode == "participant-create");
            ready(ready_path);
            return wait_for(release_path) ? 0 : 5;
        }
        if (mode == "recover-hold") {
            RuntimeRecoveryGuard recovery;
            std::string reason;
            const auto health = recovery.acquire(root, &reason);
            if (health != RuntimeLifecycleHealth::Ok) {
                std::cerr << runtime_lifecycle_health_name(health) << " " << reason
                          << std::endl;
                return 4;
            }
            ready(ready_path);
            return wait_for(release_path) ? 0 : 5;
        }
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 6;
    }
    return 2;
}
