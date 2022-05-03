#include "ui.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <implot.h>

#include <glm/gtc/random.hpp>

#include <vren/vk_helpers/misc.hpp>
#include <vren/vk_helpers/shader.hpp>

#include "app.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// plot
// --------------------------------------------------------------------------------------------------------------------------------

template<typename _t>
void left_shift_array(_t* arr, size_t size, size_t pos)
{
	size_t i;
	for (i = 0; i < size - pos; i++) {
		arr[i] = arr[i + pos];
	}
	for (; i < size; i++) {
		arr[i] = 0;
	}
}

vren_demo::plot::plot()
{
	std::fill_n(m_val, VREN_DEMO_PLOT_SAMPLES_COUNT, 0);
	std::fill_n(m_val_avg, VREN_DEMO_PLOT_SAMPLES_COUNT, 0);
}

void vren_demo::plot::push(float val)
{
	m_val_sum = m_val_sum - m_val[0] + val;

	left_shift_array(m_val, VREN_DEMO_PLOT_SAMPLES_COUNT, 1);
	m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1] = val;

	m_val_min = std::min(val, m_val_min);
	m_val_max = std::max(val, m_val_max);

	left_shift_array(m_val_avg, VREN_DEMO_PLOT_SAMPLES_COUNT, 1);
	m_val_avg[VREN_DEMO_PLOT_SAMPLES_COUNT - 1] = m_val_sum / (float) VREN_DEMO_PLOT_SAMPLES_COUNT;
}

// --------------------------------------------------------------------------------------------------------------------------------
// scene_ui
// --------------------------------------------------------------------------------------------------------------------------------

vren_demo::scene_ui::scene_ui(vren::context const& ctx)
{}

void vren_demo::scene_ui::show(vren::light_array& light_array)
{
	if (ImGui::Begin("Scene UI##scene_ui", nullptr, NULL))
	{
		/* Lighting */
		ImGui::Text("Lighting");

		/* Point lights */
		ImGui::Text("Point lights");

		int point_light_count = (int) light_array.m_point_light_count;
		ImGui::SliderInt("Num.##point_lights-lighting-scene_ui", &point_light_count, 0, VREN_MAX_POINT_LIGHTS);

		if (light_array.m_point_light_count != point_light_count)
		{
			for (uint32_t i = light_array.m_point_light_count; i < point_light_count; i++)
			{
				light_array.m_point_lights[i] = {
					.m_position = {0, 0, 0},
					.m_color = {0.91f, 0.91f, 0.77f}
				};
			}
			light_array.m_point_light_count = point_light_count;

			light_array.request_point_light_buffer_sync();
		}

		ImGui::SliderFloat("Speed##point_lights-lighting-scene_ui", &m_speed, 0.0f, 10.0f);
	}

	ImGui::End();
}

// --------------------------------------------------------------------------------------------------------------------------------
// fps_ui
// --------------------------------------------------------------------------------------------------------------------------------

vren_demo::fps_ui::fps_ui()
{
	std::fill_n(m_frame_start_t, std::size(m_frame_start_t), std::numeric_limits<uint64_t>::infinity());
	std::fill_n(m_frame_end_t, std::size(m_frame_end_t), 0);
}

void vren_demo::fps_ui::notify_frame_profiling_data(
	vren_demo::profile_info const& prof_info
)
{
	if (m_paused) {
		return;
	}

	/* FPS */
	m_fps_counter++;
	if ((glfwGetTime() - m_last_fps_time) >= 1.0 || m_last_fps_time < 0)
	{
		m_fps = m_fps_counter;
		m_fps_counter = 0;
		m_last_fps_time = glfwGetTime();
	}

	/* Main pass */
	if (prof_info.m_main_pass_profiled)
	{
		float main_pass_dt = (float) (prof_info.m_main_pass_end_t - prof_info.m_main_pass_start_t) / (1000.0f * 1000.0f);
		m_main_pass_plot.push(main_pass_dt);
	}

	/* UI pass */
	if (prof_info.m_ui_pass_profiled)
	{
		float ui_pass_dt = (float) (prof_info.m_ui_pass_end_t - prof_info.m_ui_pass_start_t) / (1000.0f * 1000.0f);
		m_ui_pass_plot.push(ui_pass_dt);
	}

	/* Frame profiling */
	if (prof_info.m_frame_profiled)
	{
		float frame_dt = (float) (prof_info.m_frame_end_t - prof_info.m_frame_start_t) / (1000.0f * 1000.0f);;
		m_frame_delta_plot.push(frame_dt);

		m_frame_start_t[prof_info.m_frame_idx] = prof_info.m_frame_start_t;
		m_frame_end_t[prof_info.m_frame_idx] = prof_info.m_frame_end_t;

		// The following algorithm is used to take the temporal interval while one frame was working in parallel
		// with other frames. As a requirement for this algorithm to work, all frames starting timestamp from
		// the oldest frame to the newest *MUST BE SEQUENTIAL*.

		int start_fi = (prof_info.m_frame_idx + 1) % prof_info.m_frame_in_flight_count; // Oldest frame
		int fi = start_fi;

		uint64_t left_t = m_frame_start_t[start_fi];
		uint64_t right_t = 0;

		uint64_t tot_par_dt = 0;

		while (true)
		{
			uint64_t slice_start_t = m_frame_end_t[fi];
			uint64_t slice_end_t = m_frame_start_t[fi];

			for (int next_fi = (fi + 1) % prof_info.m_frame_in_flight_count; next_fi != fi; next_fi = (next_fi + 1) % prof_info.m_frame_in_flight_count)
			{
				slice_start_t = std::max(std::min(m_frame_start_t[next_fi], slice_start_t), m_frame_start_t[fi]);
				slice_end_t = std::min(std::max(m_frame_end_t[next_fi], slice_end_t), m_frame_end_t[fi]);
			}

			slice_start_t = std::max(right_t, slice_start_t);
			slice_end_t = std::max(right_t, slice_end_t);
			right_t = slice_end_t;

			if (slice_end_t > slice_start_t) {
				tot_par_dt += slice_end_t - slice_start_t;
			}

			fi = (fi + 1) % prof_info.m_frame_in_flight_count;
			if (fi == start_fi) {
				break;
			}
		}

		float par_perc = ((float) tot_par_dt) / (float) (right_t - left_t);
		m_frame_parallelism_plot.push(par_perc);
	}
}

void plot_ui(std::string const& plot_title, vren_demo::plot const& plot, char const* unit)
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

			ImPlot::PlotStairs((plot_title + "##val").c_str(), plot.m_val, VREN_DEMO_PLOT_SAMPLES_COUNT);
			ImPlot::PlotStairs((plot_title + "##val_avg").c_str(), plot.m_val_avg, VREN_DEMO_PLOT_SAMPLES_COUNT);

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

		ImGui::End();
	}
}

void vren_demo::fps_ui::show()
{
	if (ImGui::Begin("Frame info##frame_ui", nullptr, NULL))
	{
		ImGui::Checkbox("Pause", &m_paused);

		ImGui::Separator();

		// General
		ImGui::Text("FPS: %d", m_fps);
		ImGui::Text("Frame parallelism: %.1f%%", m_frame_parallelism_plot.get_last_value());

		ImGui::Separator();

		// Frame profiling
		ImGui::Text("Frame profiling:");

		//plot_ui("Frame parallelism##frame_parallelism-frame_ui", m_frame_parallelism_plot, "%");
		plot_ui("Frame delta##frame_delta-frame_ui", m_frame_delta_plot, "ms");

		ImGui::Separator();

		// Passes
		if (ImGui::TreeNodeEx("Passes", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Main pass: %.3f ms", m_main_pass_plot.m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1]);
			ImGui::Text("UI pass: %.3f ms", m_main_pass_plot.m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1]);

			ImGui::TreePop();
		}

		//plot_ui("Main pass##main_pass-frame_ui", m_main_pass_plot, "ms");
		//plot_ui("UI pass##ui_pass-frame_ui", m_ui_pass_plot, "ms");
	}

	ImGui::End();
}

vren_demo::ui::ui(vren_demo::app const& app) :
	m_app(&app),
	m_scene_ui(app.m_context)
{}

void vren_demo::ui::show(vren::depth_buffer_pyramid const& depth_buffer_pyramid, vren::light_array& light_array)
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
	m_scene_ui.show(light_array);

	ImGui::SetNextWindowDockID(m_left_sidebar_dock_id, ImGuiCond_Once);
	m_fps_ui.show();

	m_depth_buffer_pyramid_ui.show(depth_buffer_pyramid);

	//ImGui::ShowDemoWindow();
	//ImPlot::ShowDemoWindow();
}



