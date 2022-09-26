include_guard()

# ImPlot is an ImGui dependency and, as such, I couldn't specify it in vcpkg's manifest because it pulls imgui in vcpkg's
# installation directory and therefore overrides vren's installation

set(implot_TAG "8879c99aef760379c85d5a7a0de7f70e7af3210c")

FetchContent_Declare(
	implot
	GIT_REPOSITORY https://github.com/epezent/implot
	GIT_TAG        ${implot_TAG}
)
FetchContent_MakeAvailable(implot)

IF (NOT TARGET implot)
	add_library(implot
		${implot_SOURCE_DIR}/implot.cpp
		${implot_SOURCE_DIR}/implot.h
		${implot_SOURCE_DIR}/implot_demo.cpp
		${implot_SOURCE_DIR}/implot_internal.h
		${implot_SOURCE_DIR}/implot_items.cpp
	)
	target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})

	# ImGui
	target_include_directories(implot PRIVATE imgui)
	target_link_libraries(implot PRIVATE imgui)
ENDIF()
