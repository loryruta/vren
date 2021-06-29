cmake_minimum_required(VERSION 3.19)

project(vren_demo)

set(CMAKE_CXX_STANDARD 20)

set(VREN_HOME ".." CACHE PATH "The path to the vren project repository.")

# ------------------------------------------------------------------------------------------------
# Libraries
# ------------------------------------------------------------------------------------------------

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(GLFW_DIR third_party/glfw)

add_subdirectory(${GLFW_DIR})
set(GLFW_INCLUDE_DIR ${GLFW_DIR}/include)

# tinyobjloader
set(TINYOBJLOADER_DIR third_party/tinyobjloader)
set(TINYOBJLOADER_INCLUDE_DIR ${TINYOBJLOADER_DIR})

# ------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/camera.hpp
        src/camera.cpp
        src/obj_loader.hpp
        src/obj_loader.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(${PROJECT_NAME} vren)

# GLFW
target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} glfw)

# tinyobjloader
target_include_directories(${PROJECT_NAME} PUBLIC ${TINYOBJLOADER_INCLUDE_DIR})

# ------------------------------------------------------------------------------------------------

include("${VREN_HOME}/VRen.cmake")

copy_resources(${PROJECT_NAME} "resources/models" "resources/models")

setup_resources(${PROJECT_NAME})
