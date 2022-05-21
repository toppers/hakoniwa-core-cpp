#include "hako_string.hpp"
#include <string.h>
#include "utils/hako_assert.hpp"

std::shared_ptr<std::string> hako::utils::hako_fixed2string(const HakoFixedStringType &str)
{
    return std::make_shared<std::string>(str.data, str.len);
}

void hako::utils::hako_string2fixed(const std::string &src, HakoFixedStringType &dst)
{
    HAKO_ASSERT(src.length() <= HAKO_FIXED_STRLEN_MAX);
    memcpy(&dst.data[0], src.c_str(), src.length());
    dst.len = src.length();
    dst.data[src.length()] = 0x0;
    return;
}
