#pragma once

#include <imgui.h>

namespace vren_demo
{
	class perf_gui
	{
	private:
		ImGuiID m_dock_id;
		ImGuiID m_screen_dock_id, m_sidebar_dock_id;

		void _show_screen();
		void _show_sidebar();

	public:
		void show();
	};
}
