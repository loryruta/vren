#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include "profile.hpp"

#include "context.hpp"
#include "renderer.hpp"
#include "utils/vk_toolbox.hpp"

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
		std::shared_ptr<vren::context> m_context;
		std::shared_ptr<vren::vk_utils::toolbox> m_toolbox;
		std::shared_ptr<vren::renderer> m_renderer;

		ImGuiID m_main_dock_id;
		ImGuiID m_left_sidebar_dock_id;
		ImGuiID m_bottom_toolbar_dock_id;


	public:
		vren_demo::ui::fps_ui m_fps_ui;

		main_ui(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::vk_utils::toolbox> const& tb,
			std::shared_ptr<vren::renderer> const& renderer
		);

		void show_vk_pool_info_ui();
		void show();
	};
}
