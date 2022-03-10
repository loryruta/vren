#include "gui.hpp"

#include <imgui_internal.h>

void vren_demo::perf_gui::_show_screen()
{
	ImGui::SetNextWindowDockID(m_screen_dock_id);

	if (ImGui::Begin("##perf_gui-screen", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		ImVec2 size = ImGui::GetWindowSize();

		m_screen->show(size, [&](vren::render_target const& target) {

		});
	}

	ImGui::End();
}

void vren_demo::perf_gui::_show_sidebar()
{
	ImGui::SetNextWindowDockID(m_sidebar_dock_id);

	if (ImGui::Begin("##perf_gui-sidebar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
	{
		ImGui::Text("Lights count");
		ImGui::SliderInt("##lights_count", &m_lights_count, 0, 1000, "%d", ImGuiSliderFlags_Logarithmic);

		ImGui::Text("Light velocity");
		ImGui::SliderFloat("##light_vel", &m_light_velocity, 0.0f, 100.0f, "%.1f", ImGuiSliderFlags_Logarithmic);

		ImGui::Text("Scene repetition");
		ImGui::SliderInt("##scene_rep", &m_scene_rep, 0, 1'000'000, "%d", ImGuiSliderFlags_Logarithmic);
	}

	ImGui::End();
}

void vren_demo::perf_gui::show()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGuiWindowFlags win_flags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground;

	ImGui::Begin("##perf_gui", nullptr, win_flags);

	ImGui::PopStyleVar(3);

	m_dock_id = ImGui::GetID("perf_gui_dockspace");

	if (ImGui::DockBuilderGetNode(m_dock_id) == nullptr)
	{
		ImGui::DockBuilderRemoveNode(m_dock_id);
		ImGui::DockBuilderAddNode(m_dock_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(m_dock_id, viewport->Size);

		ImGui::DockBuilderSplitNode(m_dock_id, ImGuiDir_Right, 0.25f, &m_sidebar_dock_id, &m_screen_dock_id);

		ImGui::DockBuilderFinish(m_dock_id);
	}

	ImGui::DockSpace(m_dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

	_show_screen();
	_show_sidebar();

	ImGui::End();
}

