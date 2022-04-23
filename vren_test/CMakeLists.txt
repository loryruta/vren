set(VREN_HOME "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "The path to the vren project repository")

set(SRC
        main.cpp
        kd_tree.cpp)

add_executable(vren_test ${SRC})

# Self include
target_include_directories(vren_test PRIVATE "${CMAKE_SOURCE_DIR}")

# vren
#target_compile_definitions(vren PUBLIC VREN_LOG_LEVEL=VREN_LOG_LEVEL_DEBUG)

target_include_directories(vren_test PUBLIC "${VREN_HOME}/vren")
target_link_libraries(vren_test PRIVATE vren)

# GoogleTest
find_package(GTest CONFIG REQUIRED)
target_link_libraries(vren_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
