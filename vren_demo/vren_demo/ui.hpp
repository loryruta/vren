#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include "profile.hpp"

#include <vren/context.hpp>
#include <vren/renderer.hpp>
#include <vren/toolbox.hpp>
#include <vren/vk_helpers/shader.hpp>

#include "tinygltf_loader.hpp"

#define VREN_DEMO_PLOT_SAMPLES_COUNT 512

namespace vren_demo::ui
{
	struct plot
	{
		float m_val[VREN_DEMO_PLOT_SAMPLES_COUNT];
		float m_val_avg[VREN_DEMO_PLOT_SAMPLES_COUNT];
		float m_val_sum = 0;
		float m_val_min = std::numeric_limits<float>::infinity();
		float m_val_max = -std::numeric_limits<float>::infinity();

		plot();

		void push(float val);
	};

	// ------------------------------------------------------------------------------------------------
	// scene_ui
	// ------------------------------------------------------------------------------------------------

	// TODO should this class manage rendering?
	class scene_ui
	{
	private:
		vren_demo::tinygltf_loader m_gltf_loader;

	public:
		float m_speed = 1.0f;
		size_t m_triangles_count = 0;

		scene_ui(vren::context const& ctx);

		void show(
			vren::render_list& render_list,
			vren::light_array& light_arr
		);
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

		uint64_t m_frame_start_t[VREN_MAX_FRAMES_IN_FLIGHT];
		uint64_t m_frame_end_t[VREN_MAX_FRAMES_IN_FLIGHT];

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
	// main_ui
	// ------------------------------------------------------------------------------------------------

	class main_ui
	{
	private:
		vren::context const* m_context;
		vren::renderer const* m_renderer;

		ImGuiID
			m_main_dock_id,
			m_left_sidebar_dock_id,
			m_right_sidebar_dock_id,
			m_bottom_toolbar_dock_id;

	public:
		vren_demo::ui::fps_ui m_fps_ui;
		vren_demo::ui::scene_ui m_scene_ui;

		main_ui(vren::context const& ctx, vren::renderer const& renderer);

		void show(
			vren::render_list& render_list,
			vren::light_array& light_arr
		);
	};
}
