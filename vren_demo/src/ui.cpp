#include "ui.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <implot.h>

#include "pooling/command_pool.hpp"
#include "pooling/fence_pool.hpp"

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

vren_demo::ui::plot::plot()
{
	std::fill_n(m_val, VREN_DEMO_PLOT_SAMPLES_COUNT, 0);
	std::fill_n(m_val_avg, VREN_DEMO_PLOT_SAMPLES_COUNT, 0);
}

void vren_demo::ui::plot::push(float val)
{
	m_val_sum = m_val_sum - m_val[0] + val;

	left_shift_array(m_val, VREN_DEMO_PLOT_SAMPLES_COUNT, 1);
	m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1] = val;

	m_val_min = std::min(val, m_val_min);
	m_val_max = std::max(val, m_val_max);

	left_shift_array(m_val_avg, VREN_DEMO_PLOT_SAMPLES_COUNT, 1);
	m_val_avg[VREN_DEMO_PLOT_SAMPLES_COUNT - 1] = m_val_sum / (float) VREN_DEMO_PLOT_SAMPLES_COUNT;
}

vren_demo::ui::fps_ui::fps_ui()
{
	std::fill_n(m_frame_start_t, std::size(m_frame_start_t), std::numeric_limits<uint64_t>::infinity());
	std::fill_n(m_frame_end_t, std::size(m_frame_end_t), 0);
}

void vren_demo::ui::fps_ui::update(float dt)
{
	double cur_time = glfwGetTime();

	m_fps_counter++;

	if (m_last_fps_time < 0 || (cur_time - m_last_fps_time) >= 1.)
	{
		m_fps = m_fps_counter;
		m_fps_counter = 0;
		m_last_fps_time = cur_time;
	}
}

void vren_demo::ui::fps_ui::notify_frame_profiling_data(
	vren_demo::profile_info const& prof_info
)
{
	if (m_paused) {
		return;
	}

	if (prof_info.m_main_pass_profiled)
	{
		float main_pass_dt = (float) (prof_info.m_main_pass_end_t - prof_info.m_main_pass_start_t) / (1000.0f * 1000.0f);
		m_main_pass_plot.push(main_pass_dt);
	}

	if (prof_info.m_ui_pass_profiled)
	{
		float ui_pass_dt = (float) (prof_info.m_ui_pass_end_t - prof_info.m_ui_pass_start_t) / (1000.0f * 1000.0f);
		m_ui_pass_plot.push(ui_pass_dt);
	}

	if (prof_info.m_frame_profiled)
	{
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

void plot_ui(std::string plot_title, vren_demo::ui::plot const& plot, char const* unit)
{
	if (ImGui::CollapsingHeader(plot_title.c_str(), NULL))
	{
		if (ImGui::BeginTable((plot_title + "##plot-summary").c_str(), 5, ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn((std::string("Latest (") + unit + ")").c_str());
			ImGui::TableSetupColumn((std::string("Min (") + unit + ")").c_str());
			ImGui::TableSetupColumn((std::string("Avg (") + unit + ")").c_str());
			ImGui::TableSetupColumn((std::string("Max (") + unit + ")").c_str());
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();

			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val[VREN_DEMO_PLOT_SAMPLES_COUNT - 1]);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val_min);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val_avg[VREN_DEMO_PLOT_SAMPLES_COUNT - 1]);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val_max);

			ImGui::EndTable();
		}

		if (ImPlot::BeginPlot((plot_title + "##plot").c_str(), ImVec2(-1, 256), ImPlotFlags_CanvasOnly))
		{
			ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);

			ImPlot::SetupAxis(ImAxis_Y1, "", ImPlotAxisFlags_LockMin);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.0, ImPlotCond_Once);

			ImPlot::PlotStairs("val", plot.m_val, VREN_DEMO_PLOT_SAMPLES_COUNT);
			ImPlot::PlotStairs("val avg", plot.m_val_avg, VREN_DEMO_PLOT_SAMPLES_COUNT);

			float min_line[] = { plot.m_val_min, plot.m_val_min }; ImPlot::PlotStairs("val min", min_line, 2, VREN_DEMO_PLOT_SAMPLES_COUNT);
			float max_line[] = { plot.m_val_max, plot.m_val_max }; ImPlot::PlotStairs("val max", max_line, 2, VREN_DEMO_PLOT_SAMPLES_COUNT);

			ImPlot::EndPlot();
		}
	}
}

void vren_demo::ui::fps_ui::show()
{
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5));

	if (ImGui::Begin("Frame info##frame_ui", nullptr, NULL))
	{
		ImGui::Text("FPS: 100");
		ImGui::Text("Plot samples count: %d", VREN_DEMO_PLOT_SAMPLES_COUNT);

		ImGui::Checkbox("Pause", &m_paused);

		ImGui::Separator();

		plot_ui("Frame parallelism##frame_parallelism-frame_ui", m_frame_parallelism_plot, "%");

		ImGui::Separator();

		ImGui::Text("Passes:");

		plot_ui("Main pass##main_pass-frame_ui", m_main_pass_plot, "ms");
		plot_ui("UI pass##ui_pass-frame_ui", m_ui_pass_plot, "ms");
	}

	ImGui::End();

	ImGui::PopStyleColor();
}

vren_demo::ui::main_ui::main_ui(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<vren::renderer> const& renderer
) :
	m_context(ctx),
	m_renderer(renderer)
{}

void vren_demo::ui::main_ui::show_vk_pool_info_ui()
{
	if (ImGui::Begin("Vulkan objects pool##vk_pool_info"))
	{
		if (ImGui::BeginTable("table##vk_pool_info", 4))
		{
			/* Headers */
			ImGui::TableSetupColumn("Pool name");
			ImGui::TableSetupColumn("Acquired objects");
			ImGui::TableSetupColumn("Pooled objects");
			ImGui::TableSetupColumn("Total objects");
			ImGui::TableHeadersRow();

			/* Body */
			auto add_row = []<typename _t>(char const* title, vren::object_pool<_t> const& pool){
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text(title);
				ImGui::TableNextColumn(); ImGui::Text("%d", pool.get_acquired_objects_count());
				ImGui::TableNextColumn(); ImGui::Text("%d", pool.get_pooled_objects_count());
				ImGui::TableNextColumn(); ImGui::Text("%d", pool.get_created_objects_count());
			};

			add_row("Graphics commands", *m_context->m_graphics_command_pool);
			add_row("Transfer commands", *m_context->m_transfer_command_pool);
			add_row("Fences", *m_context->m_fence_pool);
			add_row("Material descriptors", *m_renderer->m_material_descriptor_pool);
			add_row("Light array descriptors", *m_renderer->m_light_array_descriptor_pool);

			/**/

			ImGui::EndTable();
		}
	}

	ImGui::End();
}

void vren_demo::ui::main_ui::update(float dt)
{
	m_fps_ui.update(dt);
}

void vren_demo::ui::main_ui::show()
{
	show_vk_pool_info_ui();
	m_fps_ui.show();

	ImGui::ShowDemoWindow();
	ImPlot::ShowDemoWindow();
}



