#include "ui.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>
#include <imgui_impl_vulkan.h>

#include <glm/gtc/random.hpp>

#include <vren/vk_helpers/misc.hpp>
#include <vren/vk_helpers/shader.hpp>

#include "app.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// scene_ui
// --------------------------------------------------------------------------------------------------------------------------------

template<size_t _sample_count>
void plot_ui(std::string const& plot_title, vren_demo::profiled_data<_sample_count> const& data, char const* unit)
{
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);

	if (ImGui::TreeNode(plot_title.c_str()))
	{
		/* Summary
		if (ImGui::BeginTable((plot_title + "##plot-summary").c_str(), 5, ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn((std::string("Latest (") + unit + ")").c_str());
			ImGui::TableSetupColumn((std::string("Min (") + unit + ")").c_str());
			ImGui::TableSetupColumn((std::string("Avg (") + unit + ")").c_str());
			ImGui::TableSetupColumn((std::string("Max (") + unit + ")").c_str());
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();

			ImGui::TableNextColumn(); ImGui::Text("%.3f", plot.m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1]);
			ImGui::TableNextColumn(); ImGui::Text("%.3f", plot.m_val_min);
			ImGui::TableNextColumn(); ImGui::Text("%.3f", plot.m_val_avg[VREN_DEMO_PLOT_SAMPLES_COUNT - 1]);
			ImGui::TableNextColumn(); ImGui::Text("%.3f", plot.m_val_max);

			ImGui::EndTable();
		}*/

		/* Plot */
		if (ImPlot::BeginPlot((plot_title + "##plot").c_str(), ImVec2(-1, 128), ImPlotFlags_CanvasOnly))
		{
			ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);
			ImPlot::SetupAxis(ImAxis_Y1, unit, ImPlotAxisFlags_AutoFit);

			//ImPlot::SetupAxis(ImAxis_Y1, "", ImPlotAxisFlags_LockMin);
			//ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.0, ImPlotCond_Once);

			ImPlot::PlotStairs((plot_title + "##values").c_str(), data.m_values.data(), _sample_count);
			ImPlot::PlotStairs((plot_title + "##avg").c_str(), data.m_avg.data(), _sample_count);

			//float min_line[] = { plot.m_val_min, plot.m_val_min }; ImPlot::PlotStairs("val min", min_line, 2, VREN_DEMO_PLOT_SAMPLES_COUNT);
			//float max_line[] = { plot.m_val_max, plot.m_val_max }; ImPlot::PlotStairs("val max", max_line, 2, VREN_DEMO_PLOT_SAMPLES_COUNT);

			ImPlot::EndPlot();
		}

		ImGui::TreePop();
	}
}

void vren_demo::depth_buffer_pyramid_ui::show(vren::depth_buffer_pyramid const& depth_buffer_pyramid)
{
	if (ImGui::Begin("Depth buffer pyramid##depth_buffer_pyramid", nullptr, NULL))
	{
		ImGui::Checkbox("Show", &m_show);
		ImGui::SliderInt("Level", (int32_t*) &m_selected_level, 0, depth_buffer_pyramid.get_level_count() - 1);
	}

	ImGui::End();
}

vren_demo::ui::ui(vren_demo::app& app) :
	m_app(&app)
{
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigWindowsMoveFromTitleBarOnly = true;
}

void vren_demo::ui::show_scene_window()
{
	auto& light_arr = m_app->m_light_array;

	if (ImGui::Begin("Scene UI##scene_ui", nullptr, NULL))
	{
		/* Lighting */
		ImGui::Text("Lighting");

		/* Point lights */
		ImGui::Text("Point lights");

		int point_light_count = (int) light_arr.m_point_light_count;
		ImGui::SliderInt("Num.##point_lights-lighting-scene_ui", &point_light_count, 0, VREN_MAX_POINT_LIGHTS);

		if (light_arr.m_point_light_count != point_light_count)
		{
			for (uint32_t i = light_arr.m_point_light_count; i < point_light_count; i++)
			{
				light_arr.m_point_lights[i] = {
					.m_position = {0, 0, 0},
					.m_color = {0.91f, 0.91f, 0.77f}
				};
			}
			light_arr.m_point_light_count = point_light_count;

			light_arr.request_point_light_buffer_sync();
		}

		//ImGui::SliderFloat("Speed##point_lights-lighting-scene_ui", &m_speed, 0.0f, 10.0f);
	}

	ImGui::End();
}

void vren_demo::ui::show_profiling_window()
{
	if (ImGui::Begin("Profiling##profiling", nullptr, NULL))
	{
		// General
		ImGui::Text("General");

		ImGui::Spacing();

		ImGui::Text("FPS: %d", m_app->m_fps);
		ImGui::Text("Num. of frame-in-flight: %d", VREN_MAX_FRAME_IN_FLIGHT_COUNT);

		auto swapchain = m_app->m_presenter.get_swapchain();
		if (swapchain) {
			ImGui::Text("Swapchain images: %lld", swapchain->m_images.size());
		}

		plot_ui("Frame dt##profiling-general-frame_dt", m_app->m_frame_dt, "ms");
		ImGui::Text("Frame parallelism: %.1f%% %.1f%%", m_app->m_frame_parallelism_pct.get_last_value() * 100, m_app->m_frame_parallelism_pct.get_last_avg() * 100);

		ImGui::Spacing();

		// Profiling slots
		if (ImGui::TreeNodeEx("Profiling slots"))
		{
			ImGui::Spacing();

			if (ImGui::BeginTable("##profiling-slots", 3))
			{
				ImGui::TableSetupColumn("##profiling-slots-name", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("##profiling-slots-last_val", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("##profiling-slots-last_avg", ImGuiTableColumnFlags_WidthFixed);

				for (uint32_t slot = 1; slot < ProfileSlot_Count; slot++)
				{
					auto const& profiled_data = m_app->m_delta_time_by_profile_slot.at(slot);

					ImGui::TableNextRow();

					ImGui::TableNextColumn();

					char const* slot_name = vren_demo::get_profile_slot_name(static_cast<vren_demo::profile_slot_enum_t>(slot));
					ImGui::Text("%s", slot_name);

					ImGui::TableNextColumn();
					ImGui::Text("%.3f ms", profiled_data.get_last_value() / (1000 * 1000));

					ImGui::TableNextColumn();
					ImGui::Text("%.3f ms", profiled_data.get_last_avg() / (1000 * 1000));
				}

				ImGui::EndTable();
			}

			ImGui::TreePop();
		}

		ImGui::Spacing();

		ImGui::Separator();
	}

	ImGui::End();
}

void vren_demo::ui::show_render_graph_dump_window()
{
	if (ImGui::Begin("Render-graph dump", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::InputText("Output file##render_graph_dump", m_app->m_render_graph_dump_file, sizeof(m_app->m_render_graph_dump_file));

		if (ImGui::Button("Take##render_graph_dump"))
		{
			m_app->m_take_next_render_graph_dump = true;
		}
	}

	ImGui::End();
}

void vren_demo::ui::show_camera_window()
{
	vren_demo::camera& camera = m_app->m_camera;

	glm::vec4 a(0, 0, 0, 1);
	glm::vec4 b(0, 0, 1, 1);

	if (ImGui::Begin("Camera info##camera"))
	{
		ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.m_position.x, camera.m_position.y, camera.m_position.z);
		ImGui::Text("Yaw/pitch: (%.2f, %.2f)", camera.m_yaw, camera.m_pitch);
		ImGui::SliderFloat("FOV", &camera.m_fov_y, glm::radians(1.0f), glm::radians(90.0f), "%.2f");
		ImGui::Text("Near plane: %.2f", camera.m_near_plane);
		ImGui::Text("Far plane: %.2f", camera.m_far_plane);
		ImGui::Text("Aspect ratio: %.2f", camera.m_aspect_ratio);

		ImGui::Text("Forward: (%.2f, %.2f, %.2f)", camera.get_forward().x, camera.get_forward().y, camera.get_forward().z);
		ImGui::Text("Up: (%.2f, %.2f, %.2f)", camera.get_up().x, camera.get_up().y, camera.get_up().z);
		ImGui::Text("Right: (%.2f, %.2f, %.2f)", camera.get_right().x, camera.get_right().y, camera.get_right().z);
	}

	ImGui::End();
}

void vren_demo::ui::show(
	uint32_t frame_idx,
	vren::resource_container& resource_container
)
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("##main", nullptr,
				 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
				 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
				 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground);

	ImGui::PopStyleVar(3);

	m_main_dock_id = ImGui::GetID("main-dock_id");

	if (!ImGui::DockBuilderGetNode(m_main_dock_id))
	{
		ImGui::DockBuilderRemoveNode(m_main_dock_id);
		ImGui::DockBuilderAddNode(m_main_dock_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(m_main_dock_id, viewport->Size);

		ImGuiID ctr_dock;
		m_bottom_toolbar_dock_id = ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Down, 0.15f, nullptr, &ctr_dock);

		ImGuiID rem_dock;
		m_right_sidebar_dock_id  = ImGui::DockBuilderSplitNode(ctr_dock, ImGuiDir_Right, 0.2f, nullptr, &rem_dock);
		m_left_sidebar_dock_id   = ImGui::DockBuilderSplitNode(rem_dock, ImGuiDir_Left, 0.2f, nullptr, nullptr);

		ImGui::DockBuilderFinish(m_main_dock_id);
	}

	ImGui::DockSpace(m_main_dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::End();

	ImGui::SetNextWindowDockID(m_right_sidebar_dock_id, ImGuiCond_Once);
	show_scene_window();

	ImGui::SetNextWindowDockID(m_left_sidebar_dock_id, ImGuiCond_Once);
	show_profiling_window();

	show_camera_window();

	show_render_graph_dump_window();

	m_depth_buffer_pyramid_ui.show(*m_app->m_depth_buffer_pyramid);

	//ImGui::ShowDemoWindow();
	//ImPlot::ShowDemoWindow();
}



