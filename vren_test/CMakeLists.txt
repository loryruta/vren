set(VREN_HOME "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "The path to the vren project repository")

include("${VREN_HOME}/VRen.cmake")

set(SRC
        vren_test/kd_tree.cpp

        vren_test/primitives/blelloch_scan.cpp
        vren_test/primitives/radix_sort.cpp
        vren_test/primitives/bucket_sort.cpp
        vren_test/primitives/build_bvh.cpp
        vren_test/primitives/reduce.cpp

        vren_test/gpu_test_bench.hpp
        vren_test/main.cpp
        )

function (vren_test_executable_setup TARGET_NAME)
    # Self include
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vren_test")

    # vren
    target_compile_definitions(vren PUBLIC VREN_LOG_LEVEL=VREN_LOG_LEVEL_ERROR)

    target_include_directories(${TARGET_NAME} PUBLIC "${VREN_HOME}/vren")
    target_link_libraries(${TARGET_NAME} PUBLIC vren)

    # vren resources
    setup_resources(${TARGET_NAME})
    
    # GoogleTest
    find_package(GTest CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

    # benchmark
    find_package(benchmark CONFIG REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE benchmark::benchmark)
endfunction()

# ------------------------------------------------------------------------------------------------
# Benchmark
# ------------------------------------------------------------------------------------------------

add_executable(vren_test_benchmark ${SRC})

target_compile_definitions(vren_test_benchmark PRIVATE VREN_TEST_BENCHMARK)

vren_test_executable_setup(vren_test_benchmark)

# ------------------------------------------------------------------------------------------------
# Unit tests
# ------------------------------------------------------------------------------------------------

add_executable(vren_test_unit_tests ${SRC})

target_compile_definitions(vren_test_unit_tests PRIVATE VREN_TEST_UNIT_TESTS)

vren_test_executable_setup(vren_test_unit_tests)
