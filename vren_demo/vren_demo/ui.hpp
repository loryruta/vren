#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include <vren/context.hpp>
#include <vren/renderer.hpp>
#include <vren/toolbox.hpp>
#include <vren/vk_helpers/shader.hpp>
#include <vren/depth_buffer_pyramid.hpp>

#include "scene/tinygltf_parser.hpp"

#define VREN_DEMO_PLOT_SAMPLES_COUNT 512

namespace vren_demo
{
	// Forward decl
	class app;

	// ------------------------------------------------------------------------------------------------
	// scene_ui
	// ------------------------------------------------------------------------------------------------

	// TODO should this class manage rendering?
	class scene_ui
	{
	public:
		float m_speed = 1.0f;

		scene_ui(vren::context const& ctx);

		void show(vren::light_array& light_arr);
	};

	struct depth_buffer_pyramid_ui
	{
		uint32_t m_selected_level = 0;
		bool m_show = false;

		void show(vren::depth_buffer_pyramid const& depth_buffer_pyramid);
	};

	// ------------------------------------------------------------------------------------------------
	// UI
	// ------------------------------------------------------------------------------------------------

	class ui
	{
	private:
		vren_demo::app const* m_app;

		ImGuiID
			m_main_dock_id,
			m_left_sidebar_dock_id,
			m_right_sidebar_dock_id,
			m_bottom_toolbar_dock_id;

	public:
		vren_demo::depth_buffer_pyramid_ui m_depth_buffer_pyramid_ui;
		vren_demo::scene_ui m_scene_ui;

		ui(vren_demo::app const& app);

		void show_profiling_window();

		void show(vren::depth_buffer_pyramid const& depth_buffer_pyramid, vren::light_array& light_arr);
	};
}
