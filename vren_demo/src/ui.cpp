#include "ui.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <implot.h>

#include "pooling/command_pool.hpp"
#include "pooling/fence_pool.hpp"

vren_demo::ui::fps_ui::fps_ui()
{}

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

void vren_demo::ui::fps_ui::show()
{
	float t = glfwGetTime();

	if (ImGui::Begin("Frame profiling"))
	{
		if (ImGui::BeginTable("##frame_profiling", 2))
		{
			ImGui::EndTable();
		}
	}

	ImGui::End();
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

	ImPlot::ShowDemoWindow();
}



