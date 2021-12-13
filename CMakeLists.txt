cmake_minimum_required(VERSION 3.19)

project(vren)

set(CMAKE_CXX_STANDARD 20)

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

# ------------------------------------------------------------------------------------------------
# Library
# ------------------------------------------------------------------------------------------------

add_library(vren
        src/gpu_allocator.hpp
        src/gpu_allocator.cpp
        src/render_list.cpp
        src/render_list.hpp
        src/render_object.cpp
        src/render_object.hpp
        src/renderer.hpp
        src/renderer.cpp
        src/simple_draw.hpp
        src/simple_draw.cpp
        src/config.hpp
        src/presenter.cpp
        src/presenter.hpp
        src/vk_utils.hpp
        src/vk_utils.cpp
        src/material.hpp
        src/material.cpp
        src/frame.cpp
        src/frame.hpp
        src/light.cpp
        src/light.hpp
        src/descriptor_set_pool.hpp
        src/descriptor_set_pool.cpp
)

# Vulkan
target_include_directories(vren PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vren PUBLIC ${Vulkan_LIBRARIES})

# glm
find_package(glm CONFIG REQUIRED)
target_link_libraries(vren PUBLIC glm::glm)

# VulkanMemoryAllocator
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

if (${BUILD_DEMO})
    add_subdirectory(demo)
endif()
