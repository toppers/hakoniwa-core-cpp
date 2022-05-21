#ifndef _HAKO_STRING_H_
#define _HAKO_STRING_H_

#include "types/hako_types.hpp"
#include <string>
#include <memory>

namespace hako::utils {
    std::shared_ptr<std::string> hako_fixed2string(const HakoFixedStringType &str);
    void hako_string2fixed(const std::string &src, HakoFixedStringType &dst);
}

#endif /* _HAKO_STRING_H_ */