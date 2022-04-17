set(VREN_HOME "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "The path to the vren project repository")

add_executable(vren_demo
        vren_demo/main.cpp
        vren_demo/camera.hpp
        vren_demo/camera.cpp
        vren_demo/tinygltf_loader.hpp
        vren_demo/tinygltf_loader.cpp
        vren_demo/ui.hpp
        vren_demo/ui.cpp
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

set(SHADERS_DIR "${CMAKE_CURRENT_BINARY_DIR}/resources/shaders")

add_custom_target(create_vren_demo_dirs COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR})

# Shaders
#compile_shader(SHADERS "${VREN_HOME}/vren_demo/resources/shaders/move_lights.comp" "${SHADERS_DIR}/move_lights.comp.bin")
#compile_shader(SHADERS "${VREN_HOME}/vren_demo/resources/shaders/show_lights.comp" "${SHADERS_DIR}/show_lights.comp.bin")

#add_custom_target(vren_demo_shaders DEPENDS ${SHADERS})

#add_dependencies(vren_demo_shaders create_vren_demo_dirs)
#add_dependencies(vren_demo vren_demo_shaders)

# Models
copy_resources(vren_demo "resources/models" "resources/models")
