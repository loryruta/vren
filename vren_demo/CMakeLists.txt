set(VREN_HOME ".." CACHE PATH "The path to the vren project repository")

add_executable(vren_demo
        vren_demo/main.cpp
        vren_demo/camera.hpp
        vren_demo/camera.cpp
        vren_demo/tinygltf_loader.hpp
        vren_demo/tinygltf_loader.cpp
        vren_demo/ui.hpp
        vren_demo/ui.cpp
        vren_demo/comp/move_lights.hpp
        vren_demo/comp/move_lights.cpp
        vren_demo/comp/show_lights.hpp
        vren_demo/comp/show_lights.cpp
)

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self include
target_include_directories(vren_demo PUBLIC "${VREN_HOME}/vren_demo/vren_demo")

# vren
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

copy_resources(vren_demo "resources/models" "resources/models")
setup_resources(vren_demo)
