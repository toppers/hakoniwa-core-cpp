set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)

add_subdirectory(base-procs/hako-master)
add_subdirectory(base-procs/hako-asset)
if(MSVC)
else()
add_subdirectory(base-procs/hako-cmd)
endif()