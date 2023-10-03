# glslang
FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG sdk-1.3.261.1
)
FetchContent_MakeAvailable(glslang)

# SPIRV-Headers
set(SPIRV_SKIP_TESTS ON)
set(BUILD_TESTS OFF)
FetchContent_Declare(
    SPIRV-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers
    GIT_TAG sdk-1.3.261.1
)
FetchContent_MakeAvailable(SPIRV-Headers)

# SPIRV-Tools
set(SPIRV_SKIP_TESTS ON)
set(SPIRV_SKIP_EXECUTABLES ON)
FetchContent_Declare(
    SPIRV-Tools
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools
    GIT_TAG v2023.2
)
FetchContent_MakeAvailable(SPIRV-Tools)

# shaderc
set(SHADERC_SKIP_TESTS ON)
set(SHADERC_SKIP_EXAMPLES ON)
FetchContent_Declare(
    shaderc
    GIT_REPOSITORY https://github.com/google/shaderc
    GIT_TAG v2023.6
)
FetchContent_MakeAvailable(shaderc)
