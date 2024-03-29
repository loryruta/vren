set(VREN_HOME "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "The path to the vren project repository")

include("${VREN_HOME}/VRen.cmake")

add_executable(vren_demo
        vren_demo/main.cpp
        vren_demo/app.hpp
        vren_demo/app.cpp
        vren_demo/ui.hpp
        vren_demo/ui.cpp
        vren_demo/clusterized_model_debugger.cpp
        vren_demo/clusterized_model_debugger.hpp
        vren_demo/blit_depth_buffer_pyramid.hpp
        vren_demo/blit_depth_buffer_pyramid.cpp
        vren_demo/point_light_bouncer.hpp
        vren_demo/point_light_bouncer.cpp
        vren_demo/visualize_bvh.hpp
        vren_demo/visualize_bvh.cpp
        vren_demo/camera_controller.hpp
        vren_demo/camera_controller.cpp
        vren_demo/clustered_shading_debug.hpp
        vren_demo/clustered_shading_debug.cpp
    )

# ------------------------------------------------------------------------------------------------
# ImGui GLFW backend binary
# ------------------------------------------------------------------------------------------------

set(IMGUI_BACKENDS_BASE_URL https://raw.githubusercontent.com/ocornut/imgui/${imgui_TAG}/backends)

set(IMGUI_GLFW_BACKEND ${CMAKE_CURRENT_BINARY_DIR}/imgui_glfw_backend)

file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_glfw.cpp ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.cpp)
file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_glfw.h ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.h)

add_library(vren_demo_imgui_glfw_backend
        ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.cpp
        ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.h)

# ImGui
target_link_libraries(vren_demo_imgui_glfw_backend PRIVATE imgui)

# glfw3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren_demo_imgui_glfw_backend PRIVATE glfw)

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren_demo PRIVATE "${VREN_HOME}/vren_demo/vren_demo")

target_compile_definitions(vren_demo PRIVATE VREN_LOG_LEVEL=VREN_LOG_LEVEL_DEBUG)

target_include_directories(vren_demo PRIVATE "${VREN_HOME}/vren")
target_link_libraries(vren_demo PRIVATE vren)

# glfw3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren_demo PRIVATE glfw)

# ImGui GLFW backend
target_include_directories(vren_demo PRIVATE ${IMGUI_GLFW_BACKEND})
target_link_libraries(vren_demo PRIVATE vren_demo_imgui_glfw_backend)

# tinyfiledialogs
find_package(tinyfiledialogs CONFIG REQUIRED)
target_link_libraries(vren_demo PRIVATE tinyfiledialogs::tinyfiledialogs)

# ------------------------------------------------------------------------------------------------
# Resources
# ------------------------------------------------------------------------------------------------

setup_resources(vren_demo)

# Models
#copy_resources(vren_demo "resources/models" "resources/models")

# Shaders
compile_shader(SHADERS "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/blit_depth_buffer_pyramid.comp" "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/blit_depth_buffer_pyramid.comp.spv")
compile_shader(SHADERS "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/bounce_point_lights.comp" "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/bounce_point_lights.comp.spv")
compile_shader(SHADERS "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/fill_point_light_debug_draw_buffer.comp" "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/fill_point_light_debug_draw_buffer.comp.spv")
compile_shader(SHADERS "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/show_bvh.comp" "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/show_bvh.comp.spv")
compile_shader(SHADERS "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/show_clusters.comp" "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/show_clusters.comp.spv")
compile_shader(SHADERS "${CMAKE_CURRENT_LIST_DIR}/resources/shaders/show_clusters_geometry.comp" "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders/show_clusters_geometry.comp.spv")

add_custom_target(vren_demo_shaders DEPENDS ${SHADERS})
add_dependencies(vren_demo vren_demo_shaders)
