#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include "profile.hpp"

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

	struct plot
	{
		float m_val[VREN_DEMO_PLOT_SAMPLES_COUNT];
		float m_val_avg[VREN_DEMO_PLOT_SAMPLES_COUNT];
		float m_val_sum = 0;
		float m_val_min = std::numeric_limits<float>::infinity();
		float m_val_max = -std::numeric_limits<float>::infinity();

		plot();

		void push(float val);

		inline float get_last_value() const
		{
			return m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1];
		}
	};

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
	// fps_ui
	// ------------------------------------------------------------------------------------------------

	class fps_ui
	{
	private:
		uint32_t m_fps_counter = 0;
		uint32_t m_fps;
		double m_last_fps_time = -1.0;

		uint64_t m_frame_start_t[VREN_MAX_FRAME_IN_FLIGHT_COUNT];
		uint64_t m_frame_end_t[VREN_MAX_FRAME_IN_FLIGHT_COUNT];

		bool m_paused = false;

		plot m_frame_parallelism_plot;
		plot m_frame_delta_plot;

		plot m_main_pass_plot;
		plot m_ui_pass_plot;

	public:
		fps_ui();

		void notify_frame_profiling_data(vren_demo::profile_info const& prof_info);

		void show();
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
		vren_demo::fps_ui m_fps_ui;
		vren_demo::scene_ui m_scene_ui;

		ui(vren_demo::app const& app);

		void show(vren::depth_buffer_pyramid const& depth_buffer_pyramid, vren::light_array& light_arr);
	};
}
