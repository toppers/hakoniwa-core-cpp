set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-master
    LANGUAGES C CXX
)

find_package(spdlog REQUIRED)

add_executable(
    hako-master
    src/hako_master.cpp
)

target_link_libraries(
    hako-master 
    hako
    -pthread
    spdlog::spdlog
)

#message(STATUS "HAKO_INTERFACE_DIR=" ${HAKO_INTERFACE_DIR})

target_include_directories(
    hako-master 
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INTERFACE_DIR}
)
