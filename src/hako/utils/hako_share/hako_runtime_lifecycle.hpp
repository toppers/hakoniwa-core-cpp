#ifndef _HAKO_RUNTIME_LIFECYCLE_HPP_
#define _HAKO_RUNTIME_LIFECYCLE_HPP_

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace hako::utils::lifecycle {

enum class RuntimeLifecycleHealth {
    Ok,
    Busy,
    Unknown,
};

class RuntimeLifecycleError : public std::runtime_error {
public:
    RuntimeLifecycleError(RuntimeLifecycleHealth health, const std::string& reason)
        : std::runtime_error(reason), health_(health) {}
    RuntimeLifecycleHealth health() const noexcept { return health_; }
private:
    RuntimeLifecycleHealth health_;
};

class RuntimeParticipantLease {
public:
    RuntimeParticipantLease();
    ~RuntimeParticipantLease();
    RuntimeParticipantLease(const RuntimeParticipantLease&) = delete;
    RuntimeParticipantLease& operator=(const RuntimeParticipantLease&) = delete;

    void join(const std::string& root, bool create_session);
    uint64_t generation() const noexcept;
    bool joined() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class RuntimeRecoveryGuard {
public:
    RuntimeRecoveryGuard();
    ~RuntimeRecoveryGuard();
    RuntimeRecoveryGuard(const RuntimeRecoveryGuard&) = delete;
    RuntimeRecoveryGuard& operator=(const RuntimeRecoveryGuard&) = delete;

    RuntimeLifecycleHealth acquire(const std::string& root, std::string *reason);
    void release() noexcept;
    bool acquired() const noexcept;
    uint64_t generation() const noexcept;
    const std::vector<std::string>& inactive_leases() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

const char* runtime_lifecycle_health_name(RuntimeLifecycleHealth health) noexcept;

} // namespace hako::utils::lifecycle

#endif /* _HAKO_RUNTIME_LIFECYCLE_HPP_ */
