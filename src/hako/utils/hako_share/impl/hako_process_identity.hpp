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

bool hako_current_process_identity(HakoProcessIdentity *identity);
HakoProcessIdentityStatus hako_check_process_identity(
    const HakoProcessIdentity& identity);

#endif /* _HAKO_PROCESS_IDENTITY_HPP_ */
