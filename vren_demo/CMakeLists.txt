set(VREN_HOME "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "The path to the vren project repository")

add_executable(vren_demo
        vren_demo/main.cpp
        vren_demo/camera.hpp
        vren_demo/camera.cpp
        vren_demo/ui.hpp
        vren_demo/ui.cpp
        vren_demo/scene/intermediate_scene.hpp
        vren_demo/scene/scene_gpu_uploader.cpp
        vren_demo/scene/scene_gpu_uploader.hpp
        vren_demo/scene/tinygltf_parser.hpp
        vren_demo/scene/tinygltf_parser.cpp
)

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren_demo PUBLIC "${VREN_HOME}/vren_demo/vren_demo")

# vren
target_compile_definitions(vren PUBLIC VREN_LOG_LEVEL=VREN_LOG_LEVEL_DEBUG)

target_include_directories(vren_demo PUBLIC "${VREN_HOME}/vren")
target_link_libraries(vren_demo PUBLIC vren)

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
