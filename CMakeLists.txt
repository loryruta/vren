cmake_minimum_required(VERSION 3.19)

project(vren)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
        src/render_list.cpp
        src/render_list.hpp
        src/render_object.cpp
        src/render_object.hpp
        src/context.hpp
        src/context.cpp
        src/renderer.hpp
        src/renderer.cpp
        src/simple_draw.hpp
        src/simple_draw.cpp
        src/config.hpp
        src/presenter.cpp
        src/presenter.hpp
        src/material.hpp
        src/material.cpp
        src/frame.cpp
        src/frame.hpp
        src/light.cpp
        src/light.hpp
        src/descriptor_set_pool.hpp
        src/descriptor_set_pool.cpp
        src/utils/buffer.hpp
        src/utils/buffer.cpp
        src/utils/image.hpp
        src/utils/image.cpp
        src/utils/vk_raii.hpp
        src/utils/vk_raii.cpp
        src/debug_gui.hpp
        src/debug_gui.cpp
)

target_include_directories(vren PUBLIC src)

# Vulkan
target_include_directories(vren PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vren PUBLIC ${Vulkan_LIBRARIES})

# glm
find_package(glm CONFIG REQUIRED)
target_link_libraries(vren PUBLIC glm::glm)

# VulkanMemoryAllocator
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

# imgui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren PUBLIC imgui::imgui)

# implot
find_package(implot CONFIG REQUIRED)
target_link_libraries(vren PUBLIC implot::implot)

add_subdirectory(test)

if (${BUILD_DEMO})
    add_subdirectory(demo)
endif()
