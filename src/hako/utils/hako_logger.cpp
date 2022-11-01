#include "utils/hako_logger.hpp"

#include "spdlog/sinks/basic_file_sink.h"

void hako::utils::logger::init(const std::string &id)
{
    char logdir_path[4096];
    sprintf(logdir_path, "%s/", HAKO_LOGGER_DIRPATH);
#ifdef WIN32 
    wchar_t win_logdir_path[4096];
    mbstowcs(win_logdir_path, logdir_path, sizeof(logdir_path));
    (void)_wmkdir(win_logdir_path);
#else
    (void)mkdir(logdir_path, 0777);
#endif
    std::string logfile_path = logdir_path + id + HAKO_LOGGER_FILE_EXTENSION;
    spdlog::basic_logger_mt(id, logfile_path);
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::get(id)->info("hako logger[{0}] initialized", id);
    return;
}

std::shared_ptr<spdlog::logger> hako::utils::logger::get(const std::string &id)
{
    return spdlog::get(id);
}
