set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-command
    LANGUAGES C CXX
)


add_executable(
    hako-cmd
    src/hako_cmd.cpp
)

target_link_libraries(
    hako-cmd 
    hako
    -pthread
)

#message(STATUS "HAKO_INTERFACE_DIR=" ${HAKO_INTERFACE_DIR})

target_include_directories(
    hako-cmd 
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INTERFACE_DIR}
)
