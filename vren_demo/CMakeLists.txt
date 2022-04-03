set(VREN_HOME ".." CACHE PATH "The path to the vren project repository.")

include(FetchContent)

# tinygltfloader
FetchContent_Declare(
        tinygltfloader
        GIT_REPOSITORY https://github.com/syoyo/tinygltf
        GIT_TAG        v2.5.0
)
FetchContent_MakeAvailable(tinygltfloader)

# ------------------------------------------------------------------------------------------------
# Libraries
# ------------------------------------------------------------------------------------------------

add_executable(vren_demo
        src/main.cpp
        src/camera.hpp
        src/camera.cpp
        src/tinygltf_loader.hpp
        src/tinygltf_loader.cpp
        src/ui.hpp
        src/ui.cpp
        src/comp/move_lights.hpp
        src/comp/move_lights.cpp
        src/comp/show_lights.hpp
        src/comp/show_lights.cpp
)

# vren_demo
target_include_directories(vren_demo PUBLIC "${VREN_HOME}/vren_demo/src")

# vren
target_include_directories(vren_demo PUBLIC "${VREN_HOME}/vren/src")
target_link_libraries(vren_demo PUBLIC vren)

# tinygltfloader
target_include_directories(vren_demo PUBLIC ${tinygltfloader_SOURCE_DIR})

# glfw3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren_demo PUBLIC glfw)

# ------------------------------------------------------------------------------------------------

include("${VREN_HOME}/VRen.cmake")

copy_resources(vren_demo "resources/models" "resources/models")
setup_resources(vren_demo)
