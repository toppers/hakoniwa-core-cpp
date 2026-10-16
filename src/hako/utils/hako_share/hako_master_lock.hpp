#ifndef _HAKO_MASTER_LOCK_HPP_
#define _HAKO_MASTER_LOCK_HPP_

#include "types/hako_types.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace hako::utils::sem::flock {

enum class MasterLockHealth {
    Ok,
    Abort,
    Unknown,
};

enum class MasterLockUnknownCause {
    None,
    Identity,
    Format,
    Corrupt,
    Unsupported,
    Contention,
    System,
};

enum class MasterLockState : uint32_t {
    Unlocked = 0,
    Acquiring = 1,
    Locked = 2,
    Releasing = 3,
};

struct MasterLockInspection {
    MasterLockHealth health = MasterLockHealth::Unknown;
    MasterLockState state = MasterLockState::Unlocked;
    int32_t counter = -1;
    int64_t owner_pid = 0;
    uint64_t owner_start_token = 0;
    uint64_t generation = 0;
    MasterLockUnknownCause unknown_cause = MasterLockUnknownCause::None;
    std::string reason;
};

class MasterLockStateError : public std::runtime_error {
public:
    MasterLockStateError(
        MasterLockHealth health,
        const std::string& message,
        MasterLockUnknownCause unknown_cause = MasterLockUnknownCause::None)
        : std::runtime_error(message), health_(health),
          unknown_cause_(unknown_cause)
    {
    }

    MasterLockHealth health() const noexcept { return health_; }
    MasterLockUnknownCause unknown_cause() const noexcept
    {
        return unknown_cause_;
    }

private:
    MasterLockHealth health_;
    MasterLockUnknownCause unknown_cause_;
};

/*
 * Inspect the mmap master lock without taking the logical master lock.
 * By default the OS file lock is attempted once. force_wait=true allows a
 * bounded wait controlled by timeout_ms.
 */
MasterLockInspection inspect_master_lock(uint32_t timeout_ms, bool force_wait);

const char* master_lock_health_name(MasterLockHealth health);
const char* master_lock_state_name(MasterLockState state);
const char* master_lock_unknown_cause_name(MasterLockUnknownCause cause);

} // namespace hako::utils::sem::flock

#endif /* _HAKO_MASTER_LOCK_HPP_ */
