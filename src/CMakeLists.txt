set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)

add_library(
    hako STATIC
    hako/hako_impl.cpp
    hako/hako_asset_impl.cpp
    hako/hako_simevent_impl.cpp
    hako/hako_master_impl.cpp
    hako/core/simulation/time/hako_time.cpp
    hako/core/rpc/hako_internal_rpc.cpp
    hako/utils/hako_share/impl/hako_shared_memory.cpp
    hako/utils/hako_share/impl/hako_sem.cpp
    hako/utils/hako_string.cpp
    hako/utils/hako_logger.cpp
)

target_include_directories(
    hako
    PRIVATE /usr/local/include
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${PROJECT_SOURCE_DIR}/hako
)
