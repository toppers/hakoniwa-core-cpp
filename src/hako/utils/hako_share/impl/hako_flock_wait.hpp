#ifndef _HAKO_FLOCK_WAIT_HPP_
#define _HAKO_FLOCK_WAIT_HPP_

#include <cstdint>
#include <csignal>
#include <stdexcept>

enum class HakoFlockWaitErrorType {
    Timeout,
    Interrupted,
    SystemError,
};

class HakoFlockWaitError : public std::runtime_error {
public:
    HakoFlockWaitError(HakoFlockWaitErrorType type, const char* message)
        : std::runtime_error(message), type_(type)
    {
    }

    HakoFlockWaitErrorType type() const noexcept
    {
        return type_;
    }

private:
    HakoFlockWaitErrorType type_;
};

struct HakoFlockWaitOptions {
    uint32_t timeout_ms;
    const volatile sig_atomic_t* interrupted;
};

/*
 * Enables bounded file-lock waits for the current thread only. Existing
 * callers which do not create this scope retain the historical blocking
 * behavior.
 */
class HakoFlockWaitScope {
public:
    explicit HakoFlockWaitScope(const HakoFlockWaitOptions& options);
    ~HakoFlockWaitScope();

    HakoFlockWaitScope(const HakoFlockWaitScope&) = delete;
    HakoFlockWaitScope& operator=(const HakoFlockWaitScope&) = delete;

private:
    bool had_previous_;
    HakoFlockWaitOptions previous_;
};

bool hako_flock_wait_is_enabled();
uint64_t hako_flock_wait_now_ms();
void hako_flock_wait_checkpoint(uint64_t started_at_ms);

#endif /* _HAKO_FLOCK_WAIT_HPP_ */
