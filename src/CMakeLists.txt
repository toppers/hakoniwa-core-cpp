set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)

set(OS_TYPE "posix")
if(WIN32)
    set(OS_TYPE "win")
elseif(MSYS)
    set(OS_TYPE "win")
endif(WIN32)
MESSAGE(STATUS "OS_TYPE=" ${OS_TYPE})

include(FetchContent)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EXISTS "/etc/os-release")
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
else()
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
endif()
FetchContent_MakeAvailable(json)

add_library(
    hako STATIC
    hako/hako_impl.cpp
    hako/hako_asset_impl.cpp
    hako/hako_simevent_impl.cpp
    hako/hako_master_impl.cpp
    hako/core/simulation/time/hako_time.cpp
    hako/core/rpc/hako_internal_rpc.cpp
    #for filelock && mmap
    hako/utils/hako_share/impl/hako_sem_flock.cpp
    hako/utils/hako_share/impl/hako_shared_memory_mmap.cpp
    hako/utils/hako_share/impl/${OS_TYPE}/hako_mmap.cpp
    hako/utils/hako_share/impl/${OS_TYPE}/hako_flock.cpp

    #for sem && shared memory
    hako/utils/hako_share/impl/hako_shared_memory_factory.cpp
    
    hako/utils/hako_share/impl/${OS_TYPE}/os_file_io.cpp
    hako/utils/hako_string.cpp
    hako/utils/hako_logger.cpp
)
# POSIX特有のファイルをWIN32ではない場合にのみ追加
if(NOT WIN32)
    target_sources(hako PRIVATE
        hako/utils/hako_share/impl/posix/hako_shared_memory_shm.cpp
        hako/utils/hako_share/impl/posix/hako_sem.cpp
    )
endif()

target_include_directories(
    hako
    PRIVATE /usr/local/include
    PRIVATE /mingw64/include
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hako
    PRIVATE ${nlohmann_json_SOURCE_DIR}/single_include
)
if (MSVC)
elseif(WIN32)
    target_link_libraries(hako stdc++)
else()
endif()
