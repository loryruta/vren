
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

        vren/vk_helpers/buffer.hpp
        vren/vk_helpers/buffer.cpp
        vren/vk_helpers/image.hpp
        vren/vk_helpers/image.cpp
        vren/vk_helpers/misc.hpp
        vren/vk_helpers/misc.cpp
        vren/vk_helpers/vk_raii.hpp
        vren/vk_helpers/vk_ext.cpp
        vren/vk_helpers/vk_ext.hpp
        vren/vk_helpers/image_layout_transitions.hpp
        vren/vk_helpers/image_layout_transitions.cpp

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
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren PUBLIC vren)

# Vulkan
target_include_directories(vren PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vren PUBLIC ${Vulkan_LIBRARIES})

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

# spirv-reflect
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
target_link_libraries(vren PUBLIC unofficial::spirv-reflect::spirv-reflect)

# meshoptimizer
find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(vren PUBLIC meshoptimizer::meshoptimizer)
