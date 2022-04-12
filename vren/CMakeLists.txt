include(FetchContent)

set(SRC
        vren/base/resource_container.hpp

        vren/pools/command_pool.hpp
        vren/pools/command_pool.cpp
        vren/pools/descriptor_pool.hpp
        vren/pools/descriptor_pool.cpp
        vren/pools/object_pool.hpp

        vren/utils/command_graph.hpp
        vren/utils/command_graph.cpp
        vren/utils/profiler.hpp
        vren/utils/profiler.cpp
        vren/utils/shapes.hpp
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

        vren/mesh.cpp
        vren/mesh.hpp
        vren/texture.hpp
        vren/imgui_renderer.hpp
        vren/imgui_renderer.cpp
        vren/toolbox.hpp
        vren/toolbox.cpp
        vren/vk_helpers/shader.hpp
        vren/vk_helpers/shader.cpp
        vren/vk_helpers/barrier.hpp
        vren/dbg_renderer.hpp
        vren/dbg_renderer.cpp
        vren/render_list.cpp
        vren/render_list.hpp
        vren/render_object.cpp
        vren/render_object.hpp
        vren/context.hpp
        vren/context.cpp
        vren/renderer.hpp
        vren/renderer.cpp
        vren/simple_draw.hpp
        vren/simple_draw.cpp
        vren/config.hpp
        vren/presenter.cpp
        vren/presenter.hpp
        vren/material.hpp
        vren/material.cpp
        vren/light_array.cpp
        vren/light_array.hpp
)

add_library(vren ${SRC})

# ------------------------------------------------------------------------------------------------
# ImGui backend
# ------------------------------------------------------------------------------------------------

set(IMGUI_BACKENDS_BASE_URL https://raw.githubusercontent.com/ocornut/imgui/377b864bb4a9ed9a25674b04f64f63baf221060d/backends/)

set(IMGUI_BACKEND_DIR ${CMAKE_CURRENT_BINARY_DIR}/imgui_backend)

file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_vulkan.cpp ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp)
file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_vulkan.h   ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.h)
file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_glfw.cpp   ${IMGUI_BACKEND_DIR}/imgui_impl_glfw.cpp)
file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_glfw.h     ${IMGUI_BACKEND_DIR}/imgui_impl_glfw.h)

set(IMGUI_BACKEND_SRC
        ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_vulkan.h
        ${IMGUI_BACKEND_DIR}/imgui_impl_glfw.cpp
        ${IMGUI_BACKEND_DIR}/imgui_impl_glfw.h
        )

add_library(vren_imgui_backend ${IMGUI_BACKEND_SRC})

target_compile_definitions(vren_imgui_backend PUBLIC IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

find_package(Vulkan REQUIRED)
target_include_directories(vren_imgui_backend PRIVATE ${Vulkan_INCLUDE_DIRS})
#target_link_libraries(vren_imgui_backend PRIVATE ${Vulkan_LIBRARIES})

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren_imgui_backend PRIVATE glfw)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren_imgui_backend PRIVATE imgui::imgui)

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren PUBLIC vren)

# Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(vren PUBLIC ${Vulkan_INCLUDE_DIRS})
#target_link_libraries(vren PUBLIC ${Vulkan_LIBRARIES})

# glm
find_package(glm CONFIG REQUIRED)
target_link_libraries(vren PUBLIC glm::glm)

# glfw3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren PRIVATE glfw)

# VulkanMemoryAllocator
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

# ImGui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren PUBLIC imgui::imgui)

# ImGui backend
target_include_directories(vren PUBLIC ${IMGUI_BACKEND_DIR})
target_link_libraries(vren PUBLIC vren_imgui_backend)

# ImPlot
find_package(implot CONFIG REQUIRED)
target_link_libraries(vren PUBLIC implot::implot)

# spirv-reflect
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::spirv-reflect::spirv-reflect)

# meshoptimizer
find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(vren PUBLIC meshoptimizer::meshoptimizer)

# fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(vren PUBLIC fmt::fmt)

# volk
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk
        GIT_TAG        1.2.198
)
FetchContent_MakeAvailable(volk)
target_link_libraries(vren PUBLIC volk::volk)
