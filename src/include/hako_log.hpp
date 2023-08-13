#ifndef _HAKO_LOG_HPP_
#define _HAKO_LOG_HPP_

namespace hako::log {
    void add(const char* level, const char* file, int line, const char* function, const char* format, ...);
}

/*
 * 注意：ロック区間で本ログAPIを利用しないこと！！デッドロックします。
 */
#define HAKO_LOG_INFO(...)    hako::log::add("INFO", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define HAKO_LOG_WARN(...)    hako::log::add("WARN", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define HAKO_LOG_ERROR(...)   hako::log::add("ERROR", __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

#endif /* _HAKO_LOG_HPP_ */
