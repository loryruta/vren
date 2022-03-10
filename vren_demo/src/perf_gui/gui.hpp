#pragma once

#include <memory>

#include <imgui.h>

#include "utils/screen.hpp"

namespace vren_demo
{
	class perf_gui
	{
	private:
		ImGuiID m_dock_id;
		ImGuiID m_screen_dock_id, m_sidebar_dock_id;

		int m_lights_count = 1;
		float m_light_velocity = 1.0f;
		int m_scene_rep = 10;

		std::unique_ptr<screen> m_screen;

		void _show_screen();
		void _show_sidebar();

	public:
		void show();
	};
}
