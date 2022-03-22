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
	std::fill_n(m_val, VREN_DEMO_PLOT_SAMPLE_COUNT, 0);
	std::fill_n(m_val_avg, VREN_DEMO_PLOT_SAMPLE_COUNT, 0);
}

void vren_demo::ui::plot::push(float val)
{
	m_val_sum = m_val_sum - m_val[0] + val;

	left_shift_array(m_val, VREN_DEMO_PLOT_SAMPLE_COUNT, 1);
	m_val[VREN_DEMO_PLOT_SAMPLE_COUNT - 1] = val;

	m_val_min = std::min(val, m_val_min);
	m_val_max = std::max(val, m_val_max);

	left_shift_array(m_val_avg, VREN_DEMO_PLOT_SAMPLE_COUNT, 1);
	m_val_avg[VREN_DEMO_PLOT_SAMPLE_COUNT - 1] = m_val_sum / (float) VREN_DEMO_PLOT_SAMPLE_COUNT;
}

vren_demo::ui::fps_ui::fps_ui()
{
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

void vren_demo::ui::fps_ui::notify_frame_profiling_data(vren_demo::profile_info const& prof_info)
{
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
}

void write_row_for_plot(std::string plot_title, vren_demo::ui::plot const& plot)
{
	if (ImGui::CollapsingHeader(plot_title.c_str(), NULL))
	{
		if (ImGui::BeginTable((plot_title + "##plot-summary").c_str(), 5, ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Latest (ms)");
			ImGui::TableSetupColumn("Min (ms)");
			ImGui::TableSetupColumn("Avg (ms)");
			ImGui::TableSetupColumn("Max (ms)");
			ImGui::TableSetupColumn("Samples count");
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();

			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val[VREN_DEMO_PLOT_SAMPLE_COUNT - 1]);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val_min);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val_avg[VREN_DEMO_PLOT_SAMPLE_COUNT - 1]);
			ImGui::TableNextColumn(); ImGui::Text("%.2f", plot.m_val_max);
			ImGui::TableNextColumn(); ImGui::Text("%d", VREN_DEMO_PLOT_SAMPLE_COUNT);

			ImGui::EndTable();
		}

		if (ImPlot::BeginPlot((plot_title + "##plot").c_str(), ImVec2(-1, 256), ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText))
		{
			ImPlot::SetupAxis(ImAxis_X1, "", ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoDecorations);

			ImPlot::SetupAxis(ImAxis_Y1, "", ImPlotAxisFlags_LockMin);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1.0, ImPlotCond_Once);

			ImPlot::PlotLine("val", plot.m_val, VREN_DEMO_PLOT_SAMPLE_COUNT);
			ImPlot::PlotLine("val avg", plot.m_val_avg, VREN_DEMO_PLOT_SAMPLE_COUNT);

			float min_line[] = { plot.m_val_min, plot.m_val_min }; ImPlot::PlotLine("val min", min_line, 2, VREN_DEMO_PLOT_SAMPLE_COUNT);
			float max_line[] = { plot.m_val_max, plot.m_val_max }; ImPlot::PlotLine("val max", max_line, 2, VREN_DEMO_PLOT_SAMPLE_COUNT);

			ImPlot::EndPlot();
		}
	}
}

void vren_demo::ui::fps_ui::show()
{
	if (ImGui::Begin("Frame profiling##fps_ui", nullptr))
	{
		write_row_for_plot("Main pass", m_main_pass_plot);

		write_row_for_plot("UI pass", m_ui_pass_plot);

		ImGui::End();
	}
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

			add_row("Graphics command pool", *m_context->m_graphics_command_pool);
			add_row("Transfer command pool", *m_context->m_transfer_command_pool);
			add_row("Fence pool", *m_context->m_fence_pool);

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



