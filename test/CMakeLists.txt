set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)

find_package(GTest REQUIRED)

include(GoogleTest)

add_executable(
    hako-test
    hako/hako_shared_memory_test.cpp
    hako/hako_test.cpp
    hako/hako_time_test.cpp
    hako/main.cpp
)

target_include_directories(
    hako-test
    PRIVATE ${HAKO_INTERFACE_DIR}
    PRIVATE ${HAKO_SOURCE_DIR}
    PRIVATE ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(hako-test
    hako
    -pthread
    GTest::GTest
)

gtest_add_tests(TARGET hako-test)
