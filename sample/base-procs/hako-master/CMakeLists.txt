set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-master
    LANGUAGES C CXX
)


add_executable(
    hako-master
    src/hako_master.cpp
)
if(MSVC)
target_link_libraries(
    hako-master 
    hako
)
else()
target_link_libraries(
    hako-master 
    hako
    -pthread
)
endif()


#message(STATUS "HAKO_INTERFACE_DIR=" ${HAKO_INTERFACE_DIR})

target_include_directories(
    hako-master 
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INTERFACE_DIR}
)
