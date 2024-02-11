#ifndef _HAKO_SEM_HPP_
#define _HAKO_SEM_HPP_

#include "types/hako_types.hpp"

namespace hako::utils::sem::flock {
    int32_t create(int32_t key);
    void destroy(int32_t sem_id);
    void master_lock(int32_t sem_id);
    void master_unlock(int32_t sem_id);
    void asset_down(int32_t sem_id, int32_t asset_id);
    void asset_up(int32_t sem_id, int32_t asset_id);
}

#endif /* _HAKO_SEM_HPP_ */
