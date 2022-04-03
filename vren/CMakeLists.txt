
set(SRC
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
        src/light_array.cpp
        src/light_array.hpp
        src/pooling/descriptor_pool.hpp
        src/pooling/descriptor_pool.cpp
        src/imgui_renderer.hpp
        src/imgui_renderer.cpp
        src/pooling/command_pool.hpp
        src/pooling/command_pool.cpp
        src/resource_container.hpp
        src/utils/buffer.hpp
        src/utils/buffer.cpp
        src/utils/image.hpp
        src/utils/image.cpp
        src/utils/misc.hpp
        src/utils/misc.cpp
        src/utils/vk_raii.hpp
        src/command_graph.hpp
        src/command_graph.cpp
        src/pooling/object_pool.hpp
        src/pooling/fence_pool.cpp
        src/pooling/fence_pool.hpp
        src/utils/image_layout_transitions.hpp
        src/utils/image_layout_transitions.cpp
        src/utils/profiler.hpp
        src/utils/profiler.cpp
        src/utils/vk_toolbox.hpp
        src/utils/vk_toolbox.cpp
        src/utils/shader.hpp
        src/utils/shader.cpp
        src/utils/barrier.hpp
        src/shapes.hpp
)

add_library(vren ${SRC})

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

# ImGui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren PUBLIC imgui::imgui)

# ImPlot
find_package(implot CONFIG REQUIRED)
target_link_libraries(vren PUBLIC implot::implot)

# spirv-reflect
find_package(unofficial-spirv-reflect CONFIG REQUIRED)
target_link_libraries(vren PRIVATE unofficial::spirv-reflect::spirv-reflect)
