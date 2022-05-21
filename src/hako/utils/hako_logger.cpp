#include "utils/hako_logger.hpp"
#include "spdlog/sinks/rotating_file_sink.h"

void hako::utils::logger::init(const std::string &id)
{
    char logdir_path[4096];
    sprintf(logdir_path, "%s/", HAKO_LOGGER_DIRPATH);
    (void)mkdir(logdir_path, 0777);
    std::string logfile_path = logdir_path + id + HAKO_LOGGER_FILE_EXTENSION;
    spdlog::rotating_logger_mt(id, 
        logfile_path, 
        HAKO_LOGGER_MAXSIZE, 
        HAKO_LOGGER_ROTNUM);
    spdlog::get(id)->info("hako logger[{0}] initialized", id);
    return;
}

std::shared_ptr<spdlog::logger> hako::utils::logger::get(const std::string &id)
{
    return spdlog::get(id);
}
