cmake_minimum_required(VERSION 3.19)

project(vren)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_TEST "Should build unit tests?" ON)
option(BUILD_DEMO "Should build the renderer demo?" ON)

# Vulkan
find_package(Vulkan REQUIRED)
# ${Vulkan_INCLUDE_DIRS}
# ${Vulkan_LIBRARIES}

# VCPKG
include(FetchContent)
message(STATUS "Fetching vcpkg...")
FetchContent_Declare(vcpkg URL "https://github.com/microsoft/vcpkg/archive/refs/tags/2021.05.12.tar.gz")
FetchContent_MakeAvailable(vcpkg)
include("${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake")

add_subdirectory(vren)

if (${BUILD_TEST})
    add_subdirectory(vren_test)
endif()

if (${BUILD_DEMO})
    add_subdirectory(vren_demo)
endif()
