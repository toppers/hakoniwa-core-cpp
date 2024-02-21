set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-asset
    LANGUAGES C CXX
)


add_executable(
    hako-asset
    src/hako_asset.cpp
)

if(MSVC)
target_link_libraries(
    hako-asset 
    hako
)
else()
target_link_libraries(
    hako-asset 
    hako
    -pthread
)
endif()

#message(STATUS "HAKO_INTERFACE_DIR=" ${HAKO_INTERFACE_DIR})

target_include_directories(
    hako-asset 
    PRIVATE /usr/local/include
    PRIVATE ${HAKO_INTERFACE_DIR}
)
