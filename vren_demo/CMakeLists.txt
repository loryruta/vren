set(VREN_HOME "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "The path to the vren project repository")

add_executable(vren_demo
        vren_demo/main.cpp
        vren_demo/camera.hpp
        vren_demo/camera.cpp
        vren_demo/app.hpp
        vren_demo/app.cpp
        vren_demo/ui.hpp
        vren_demo/ui.cpp
        vren_demo/scene/intermediate_scene.hpp
        vren_demo/scene/scene_gpu_uploader.cpp
        vren_demo/scene/scene_gpu_uploader.hpp
        vren_demo/scene/tinygltf_parser.hpp
        vren_demo/scene/tinygltf_parser.cpp
)

# ------------------------------------------------------------------------------------------------
# ImGui GLFW backend binary
# ------------------------------------------------------------------------------------------------

set(IMGUI_BACKENDS_BASE_URL https://raw.githubusercontent.com/ocornut/imgui/master/backends)

set(IMGUI_GLFW_BACKEND ${CMAKE_CURRENT_BINARY_DIR}/imgui_glfw_backend)

file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_glfw.cpp ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.cpp)
file(DOWNLOAD ${IMGUI_BACKENDS_BASE_URL}/imgui_impl_glfw.h ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.h)

add_library(vren_demo_imgui_glfw_backend
        ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.cpp
        ${IMGUI_GLFW_BACKEND}/imgui_impl_glfw.h
        )

# ImGui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren_demo_imgui_glfw_backend PRIVATE imgui::imgui)

# glfw3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren_demo_imgui_glfw_backend PRIVATE glfw)

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren_demo PRIVATE "${VREN_HOME}/vren_demo/vren_demo")

# vren
target_compile_definitions(vren PRIVATE VREN_LOG_LEVEL=VREN_LOG_LEVEL_DEBUG)

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

# tinygltf
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(vren_demo PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# ------------------------------------------------------------------------------------------------
# Resources
# ------------------------------------------------------------------------------------------------

include("${VREN_HOME}/VRen.cmake")
setup_resources(vren_demo)

# Models
copy_resources(vren_demo "resources/models" "resources/models")
