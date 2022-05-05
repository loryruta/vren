include(FetchContent)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

set(SRC
        vren/base/base.hpp
        vren/base/bounding_shape.cpp
        vren/base/kd_tree.cpp
        vren/base/resource_container.hpp

        vren/pools/command_pool.hpp
        vren/pools/command_pool.cpp
        vren/pools/descriptor_pool.hpp
        vren/pools/descriptor_pool.cpp
        vren/pools/object_pool.hpp

        vren/mesh/mesh.hpp
        vren/mesh/mesh_clusterizer.cpp

        vren/utils/command_graph.hpp
        vren/utils/command_graph.cpp
        vren/utils/profiler.hpp
        vren/utils/profiler.cpp
        vren/utils/log.hpp

        vren/vk_helpers/debug_utils.hpp
        vren/vk_helpers/buffer.hpp
        vren/vk_helpers/buffer.cpp
        vren/vk_helpers/image.hpp
        vren/vk_helpers/image.cpp
        vren/vk_helpers/misc.hpp
        vren/vk_helpers/misc.cpp
        vren/vk_helpers/vk_raii.hpp
        vren/vk_helpers/image_layout_transitions.hpp
        vren/vk_helpers/image_layout_transitions.cpp

        vren/depth_buffer_pyramid.cpp
        vren/gpu_repr.hpp
        vren/material.cpp
        vren/material.hpp
        vren/texture_manager.cpp
        vren/texture_manager.hpp
        vren/imgui_renderer.hpp
        vren/imgui_renderer.cpp
        vren/toolbox.hpp
        vren/toolbox.cpp
        vren/vk_helpers/shader.hpp
        vren/vk_helpers/shader.cpp
        vren/vk_helpers/barrier.hpp
        vren/debug_renderer.hpp
        vren/debug_renderer.cpp
        vren/render_graph.hpp
        vren/render_graph.cpp
        vren/context.hpp
        vren/context.cpp
        vren/basic_renderer.cpp
        vren/mesh_shader_renderer.cpp
        vren/renderer.hpp
        vren/mesh_shader_draw_pass.hpp
        vren/mesh_shader_draw_pass.cpp
        vren/config.hpp
        vren/presenter.cpp
        vren/presenter.hpp
        vren/light.cpp
        vren/light.hpp

        vren/vertex_pipeline_draw_pass.cpp
        vren/vertex_pipeline_draw_pass.hpp
)

add_library(vren ${SRC})

# ------------------------------------------------------------------------------------------------
# ImGui Vulkan backend binary
# ------------------------------------------------------------------------------------------------

set(IMGUI_BACKENDS_BASE_URL https://raw.githubusercontent.com/ocornut/imgui/master/backends)

set(IMGUI_VULKAN_BACKEND_DIR ${CMAKE_CURRENT_BINARY_DIR}/imgui_vulkan_backend)

file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_vulkan.cpp ${IMGUI_VULKAN_BACKEND_DIR}/imgui_impl_vulkan.cpp)
file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_vulkan.h ${IMGUI_VULKAN_BACKEND_DIR}/imgui_impl_vulkan.h)

add_library(vren_imgui_vulkan_backend
        ${IMGUI_VULKAN_BACKEND_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_VULKAN_BACKEND_DIR}/imgui_impl_vulkan.h
        )

target_compile_definitions(vren_imgui_vulkan_backend PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(vren_imgui_vulkan_backend PRIVATE ${Vulkan_INCLUDE_DIRS})

# ImGui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren_imgui_vulkan_backend PRIVATE imgui::imgui)

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren PUBLIC vren)

# volk
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG        1.2.198
)
FetchContent_MakeAvailable(volk)
target_link_libraries(vren PUBLIC volk::volk)

# glm
find_package(glm CONFIG REQUIRED)
target_link_libraries(vren PUBLIC glm::glm)

# VulkanMemoryAllocator
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

# ImGui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren PUBLIC imgui::imgui)

# ImPlot
find_package(implot CONFIG REQUIRED)
target_link_libraries(vren PUBLIC implot::implot)

# ImGui Vulkan backend
target_include_directories(vren PUBLIC ${IMGUI_VULKAN_BACKEND_DIR})
target_link_libraries(vren PUBLIC vren_imgui_vulkan_backend)

# spirv-reflect
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::spirv-reflect::spirv-reflect)

# meshoptimizer
find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(vren PUBLIC meshoptimizer::meshoptimizer)

# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(vren PUBLIC fmt::fmt)
