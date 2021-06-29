cmake_minimum_required(VERSION 3.19)

project(vren)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_DEMO "Should build the renderer demo?" ON)

# Vulkan
find_package(Vulkan REQUIRED)
# ${Vulkan_INCLUDE_DIRS}
# ${Vulkan_LIBRARIES}

# VulkanMemoryAllocator
set(VULKAN_MEMORY_ALLOCATOR_DIR "third_party/VulkanMemoryAllocator")
set(VULKAN_MEMORY_ALLOCATOR_INCLUDE_DIR "${VULKAN_MEMORY_ALLOCATOR_DIR}/include")

# glm
set(GLM_DIR "third_party/glm")
set(GLM_INCLUDE_DIR "${GLM_DIR}")

# ------------------------------------------------------------------------------------------------
# Library
# ------------------------------------------------------------------------------------------------

add_library(${PROJECT_NAME}
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
)

# Vulkan
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

# VulkanMemoryAllocator
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_MEMORY_ALLOCATOR_INCLUDE_DIR})

# glm
target_include_directories(${PROJECT_NAME} PUBLIC ${GLM_INCLUDE_DIR})

if (${BUILD_DEMO})
    add_subdirectory(demo)
endif()
