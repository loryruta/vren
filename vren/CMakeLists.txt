include(FetchContent)
include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

set(SRC
        vren/base/base.hpp
        vren/base/bounding_shape.cpp
        vren/base/fixed_capacity_vector.hpp
        vren/base/kd_tree.cpp
        vren/base/kd_tree.hpp
        vren/base/resource_container.hpp

        vren/model/basic_model_draw_buffer.hpp
        vren/model/basic_model_uploader.cpp
        vren/model/basic_model_uploader.hpp
        vren/model/clusterized_model.hpp
        vren/model/clusterized_model_draw_buffer.hpp
        vren/model/clusterized_model_uploader.hpp
        vren/model/clusterized_model_uploader.cpp
        vren/model/mesh.hpp
        vren/model/mesh_clusterizer.cpp
        vren/model/model.cpp
        vren/model/model.hpp
        vren/model/model_clusterizer.cpp
        vren/model/model_clusterizer.hpp
        vren/model/tinygltf_parser.cpp
        vren/model/tinygltf_parser.hpp

        vren/pipeline/basic_renderer.cpp
        vren/pipeline/basic_renderer.hpp
        vren/pipeline/mesh_shader_renderer.cpp
        vren/pipeline/mesh_shader_renderer.hpp
        vren/pipeline/render_target.hpp
        vren/pipeline/mesh_shader_draw_pass.hpp
        vren/pipeline/mesh_shader_draw_pass.cpp
        vren/pipeline/vertex_pipeline_draw_pass.cpp
        vren/pipeline/vertex_pipeline_draw_pass.hpp
        vren/pipeline/profiler.hpp
        vren/pipeline/profiler.cpp
        vren/pipeline/depth_buffer_pyramid.cpp
        vren/pipeline/imgui_renderer.hpp
        vren/pipeline/imgui_renderer.cpp
        vren/pipeline/debug_renderer.hpp
        vren/pipeline/debug_renderer.cpp
        vren/pipeline/render_graph.hpp
        vren/pipeline/render_graph.cpp

        vren/pool/command_pool.hpp
        vren/pool/command_pool.cpp
        vren/pool/descriptor_pool.hpp
        vren/pool/descriptor_pool.cpp
        vren/pool/object_pool.hpp
        
        vren/primitives/blelloch_scan.cpp
        vren/primitives/blelloch_scan.hpp
        vren/primitives/radix_sort.cpp
        vren/primitives/radix_sort.hpp
        vren/primitives/reduce.cpp
        vren/primitives/reduce.hpp

        vren/third_party/impl.cpp

        vren/vk_helpers/shader.hpp
        vren/vk_helpers/shader.cpp
        vren/vk_helpers/barrier.hpp
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

        vren/gpu_repr.hpp
        vren/material.cpp
        vren/material.hpp
        vren/texture_manager.cpp
        vren/texture_manager.hpp
        vren/toolbox.hpp
        vren/toolbox.cpp
        vren/context.hpp
        vren/context.cpp
        vren/config.hpp
        vren/presenter.cpp
        vren/presenter.hpp
        vren/light.cpp
        vren/light.hpp
        vren/log.hpp
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

# SPIRV-Cross
find_package(spirv_cross_core CONFIG REQUIRED)
target_link_libraries(vren PUBLIC spirv-cross-core)

find_package(spirv_cross_reflect CONFIG REQUIRED)
target_link_libraries(vren PUBLIC spirv-cross-reflect)

# meshoptimizer
find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(vren PUBLIC meshoptimizer::meshoptimizer)

# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(vren PUBLIC fmt::fmt)

# tinygltf
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(vren PRIVATE ${TINYGLTF_INCLUDE_DIRS})
