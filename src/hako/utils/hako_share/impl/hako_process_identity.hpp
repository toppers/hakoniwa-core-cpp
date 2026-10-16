#ifndef _HAKO_PROCESS_IDENTITY_HPP_
#define _HAKO_PROCESS_IDENTITY_HPP_

#include <cstdint>

enum class HakoProcessIdentityStatus {
    Alive,
    Dead,
    Unknown,
};

struct HakoProcessIdentity {
    int64_t pid = 0;
    uint64_t start_token = 0;
};

namespace hako::utils::process_identity::detail {

/* Pure classification seam used to keep permission/lookup failures UNKNOWN. */
HakoProcessIdentityStatus classify_lookup(
    bool definitely_missing,
    bool lookup_succeeded,
    bool terminated,
    uint64_t expected_start_token,
    uint64_t actual_start_token);

} // namespace hako::utils::process_identity::detail

bool hako_current_process_identity(HakoProcessIdentity *identity);
HakoProcessIdentityStatus hako_check_process_identity(
    const HakoProcessIdentity& identity);

#endif /* _HAKO_PROCESS_IDENTITY_HPP_ */
