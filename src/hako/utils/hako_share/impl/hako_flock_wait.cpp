#include "utils/hako_share/impl/hako_flock_wait.hpp"

#include <chrono>

namespace {
thread_local bool wait_enabled = false;
thread_local HakoFlockWaitOptions wait_options = {0, nullptr};
}

HakoFlockWaitScope::HakoFlockWaitScope(const HakoFlockWaitOptions& options)
    : had_previous_(wait_enabled), previous_(wait_options)
{
    wait_options = options;
    wait_enabled = true;
}

HakoFlockWaitScope::~HakoFlockWaitScope()
{
    wait_options = previous_;
    wait_enabled = had_previous_;
}

bool hako_flock_wait_is_enabled()
{
    return wait_enabled;
}

uint64_t hako_flock_wait_now_ms()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void hako_flock_wait_checkpoint(uint64_t started_at_ms)
{
    if (!wait_enabled) {
        return;
    }
    if ((wait_options.interrupted != nullptr) && (*wait_options.interrupted != 0)) {
        throw HakoFlockWaitError(
            HakoFlockWaitErrorType::Interrupted,
            "file-lock wait interrupted");
    }
    const uint64_t now_ms = hako_flock_wait_now_ms();
    if ((now_ms - started_at_ms) >= wait_options.timeout_ms) {
        throw HakoFlockWaitError(
            HakoFlockWaitErrorType::Timeout,
            "file-lock wait timed out");
    }
}
