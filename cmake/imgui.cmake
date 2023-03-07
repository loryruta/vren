include_guard()

# ImGui is shared between multiple subprojects (vren and vren_demo) with different features (impl_vulkan and impl_glfw).
# The version has to be known and, for this reason, can't be declared in vcpkg's manifest

set(imgui_TAG "94e850fd6ff9eceb98fda3147e3ffd4781ad2dc7")

FetchContent_Declare(
	imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui
	GIT_TAG        ${imgui_TAG}
)
FetchContent_MakeAvailable(imgui)

IF (NOT TARGET imgui)
	add_library(imgui
		${imgui_SOURCE_DIR}/imgui.cpp
		${imgui_SOURCE_DIR}/imgui.h
		${imgui_SOURCE_DIR}/imgui_demo.cpp
		${imgui_SOURCE_DIR}/imgui_draw.cpp
		${imgui_SOURCE_DIR}/imgui_internal.h
		${imgui_SOURCE_DIR}/imgui_tables.cpp
		${imgui_SOURCE_DIR}/imgui_widgets.cpp
		${imgui_SOURCE_DIR}/imstb_rectpack.h
		${imgui_SOURCE_DIR}/imstb_textedit.h
		${imgui_SOURCE_DIR}/imstb_truetype.h
		${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
		${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
		#${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.cpp
		#${imgui_SOURCE_DIR}/misc/freetype/imgui_freetype.h
	)
	target_compile_features(imgui PRIVATE "cxx_std_11")
	target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
ENDIF()
