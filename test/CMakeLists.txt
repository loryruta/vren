set(VREN_HOME ".." CACHE PATH "The path to the vren project repository.")

set(SRC
        main.cpp)

add_executable(vren_test ${SRC})

# vren
target_include_directories(vren_test PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(vren_test PUBLIC vren)

# GoogleTest
find_package(GTest CONFIG REQUIRED)
target_link_libraries(vren_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
