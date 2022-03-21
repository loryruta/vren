#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include "context.hpp"
#include "renderer.hpp"

#define VREN_DEMO_FPS_SAMPLES_LENGTH 256

namespace vren_demo::ui
{
	class fps_ui
	{
	public:
		struct frame_profiling_data
		{
			float m_main_pass_start;
			float m_main_pass_end;

			float m_ui_pass_start;
			float m_ui_pass_end;
		};

	private:
		uint32_t m_fps_counter = 0;
		uint32_t m_fps;
		double m_last_fps_time = -1.;




	public:
		fps_ui();

		void draw_frame_graph();

		void show();

		void update(float dt);

		//void push_data(frame_profiling_data& prof_data);
	};

	// ------------------------------------------------------------------------------------------------
	// main_ui
	// ------------------------------------------------------------------------------------------------

	class main_ui
	{
	private:
		std::shared_ptr<vren::context> m_context;
		std::shared_ptr<vren::renderer> m_renderer;

		vren_demo::ui::fps_ui m_fps_ui;

	public:
		main_ui(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::renderer> const& renderer
		);

		void show_vk_pool_info_ui();
		void show();

		void update(float dt);
	};


}
