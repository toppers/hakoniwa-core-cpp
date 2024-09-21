set(HAKO_CMAKE_VERSION 3.5.1)
cmake_minimum_required(VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    VERSION 1.0.0
    DESCRIPTION "hakoniwa-core project"
    LANGUAGES C CXX
)

enable_testing()

set(HAKO_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/hako")
set(HAKO_INTERFACE_DIR "${PROJECT_SOURCE_DIR}/src/include")

if (DEFINED HAKO_DATA_MAX_ASSET_NUM)
    add_compile_definitions(HAKO_DATA_MAX_ASSET_NUM=${HAKO_DATA_MAX_ASSET_NUM})
    message(STATUS "HAKO_DATA_MAX_ASSET_NUM is redefined with " ${HAKO_DATA_MAX_ASSET_NUM})
endif()

if (DEFINED HAKO_OPTION_FILEPATH)
    include(${HAKO_OPTION_FILEPATH})
else()
    include(default-cmake-options.cmake)
endif()

message(STATUS "BUILD_TYPE" ${BUILD_TYPE})
message(STATUS "GCOV" ${GCOV})

add_subdirectory(src)

if (test)
    add_subdirectory(test)
endif()

add_subdirectory(sample)

set(INSTALL_CMAKE_DIR ${PROJECT_SOURCE_DIR}/public)

install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/src/include/
    DESTINATION ${INSTALL_CMAKE_DIR}/include
)

#install(
#    TARGETS hakoniwa-core
#    DESTINATION ${INSTALL_CMAKE_DIR}
#    EXPORT  hakoniwa-core-export
#    LIBRARY DESTINATION lib
#    INCLUDES DESTINATION include
#    PUBLIC_HEADER DESTINATION include
#)

#install(
#    EXPORT hakoniwa-core-export
#    DESTINATION ${INSTALL_CMAKE_DIR}
#    EXPORT_LINK_INTERFACE_LIBRARIES
#)