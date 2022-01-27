
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
        src/utils/misc.hpp
        src/utils/misc.cpp
        src/utils/vk_raii.hpp
        src/utils/vk_raii.cpp
        src/imgui_renderer.hpp
        src/imgui_renderer.cpp
        src/command_pool.hpp
        src/command_pool.cpp
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

