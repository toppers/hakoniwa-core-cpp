set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)
find_package(spdlog REQUIRED)

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

add_library(
    hako STATIC
    hako/hako_impl.cpp
    hako/hako_asset_impl.cpp
    hako/hako_simevent_impl.cpp
    hako/hako_master_impl.cpp
    hako/core/simulation/time/hako_time.cpp
    hako/core/rpc/hako_internal_rpc.cpp
    hako/utils/hako_share/impl/posix/hako_shared_memory_mmap.cpp
    #hako/utils/hako_share/impl/posix/hako_sem.cpp
    hako/utils/hako_share/impl/${OS_TYPE}/hako_mmap.cpp
    hako/utils/hako_share/impl/${OS_TYPE}/hako_flock.cpp
    hako/utils/hako_share/impl/posix/hako_sem_flock.cpp
    hako/utils/hako_string.cpp
    hako/utils/hako_logger.cpp
)

target_include_directories(
    hako
    PRIVATE /usr/local/include
    PRIVATE /mingw64/include
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hako
    PRIVATE ${PROJECT_SOURCE_DIR}/../third-party/json/single_include
)

target_link_libraries(hako PRIVATE spdlog::spdlog)
