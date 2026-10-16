set(HAKO_CMAKE_VERSION ${HAKO_CMAKE_VERSION})

project(hakoniwa-core
    LANGUAGES C CXX
)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

include(GoogleTest)

add_executable(
    hako-test
    hako/hako_shared_memory_test.cpp
    hako/hako_flock_wait_test.cpp
    hako/hako_process_identity_test.cpp
    hako/hako_runtime_lifecycle_test.cpp
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

gtest_add_tests(TARGET hako-test TEST_LIST hako_gtests)
set_tests_properties(${hako_gtests} PROPERTIES TIMEOUT 15)

add_executable(
    hako-master-lock-process
    hako/hako_master_lock_process.cpp
)
target_include_directories(
    hako-master-lock-process
    PRIVATE ${HAKO_INTERFACE_DIR}
    PRIVATE ${HAKO_SOURCE_DIR}
)
target_link_libraries(hako-master-lock-process hako Threads::Threads)

add_test(
    NAME HakoMasterLockIntegration
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/hako/hako_master_lock_integration.py
        --helper $<TARGET_FILE:hako-master-lock-process>
)
set_tests_properties(HakoMasterLockIntegration PROPERTIES TIMEOUT 30)

add_executable(
    hako-runtime-lifecycle-process
    hako/hako_runtime_lifecycle_process.cpp
)
target_include_directories(
    hako-runtime-lifecycle-process
    PRIVATE ${HAKO_INTERFACE_DIR}
    PRIVATE ${HAKO_SOURCE_DIR}
)
target_link_libraries(hako-runtime-lifecycle-process hako Threads::Threads)

add_test(
    NAME HakoRuntimeLifecycleIntegration
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/hako/hako_runtime_lifecycle_integration.py
        --helper $<TARGET_FILE:hako-runtime-lifecycle-process>
)
set_tests_properties(HakoRuntimeLifecycleIntegration PROPERTIES TIMEOUT 30)
