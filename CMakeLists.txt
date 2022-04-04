cmake_minimum_required(VERSION 3.19)

project(vren)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_DEMO "Should build the renderer demo?" ON)
option(BUILD_TEST "Should build unit tests?" ON)

# Vulkan
find_package(Vulkan REQUIRED)
# ${Vulkan_INCLUDE_DIRS}
# ${Vulkan_LIBRARIES}

if (MSVC)
    #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
    #set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} /fsanitize=address")
endif()

add_subdirectory(vren)

if (${BUILD_TEST})
    add_subdirectory(vren_test)
endif()

if (${BUILD_DEMO})
    add_subdirectory(vren_demo)
endif()
