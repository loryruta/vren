set(VREN_HOME ".." CACHE PATH "The path to the vren project repository.")

# ------------------------------------------------------------------------------------------------
# Libraries
# ------------------------------------------------------------------------------------------------

add_executable(vren_demo
        src/main.cpp
        src/camera.hpp
        src/camera.cpp
        src/ai_scene_loader.hpp
        src/ai_scene_loader.cpp
)

# vren
target_include_directories(vren_demo PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(vren_demo PUBLIC vren)

# assimp
find_package(assimp CONFIG REQUIRED)
target_link_libraries(vren_demo PUBLIC assimp::assimp)

# glfw3
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(vren_demo PUBLIC glfw)

# imgui
find_package(imgui CONFIG REQUIRED)
target_link_libraries(vren_demo PUBLIC imgui::imgui)

# ------------------------------------------------------------------------------------------------

include("${VREN_HOME}/VRen.cmake")

copy_resources(vren_demo "resources/models" "resources/models")
setup_resources(vren_demo)
