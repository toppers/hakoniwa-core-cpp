set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

include(GoogleTest)

add_executable(
    hako-test
    hako/hako_shared_memory_test.cpp
    hako/hako_flock_wait_test.cpp
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
    Threads::Threads
    GTest::GTest
)

gtest_add_tests(TARGET hako-test)
