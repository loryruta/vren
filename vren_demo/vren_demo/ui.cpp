#include "ui.hpp"

#include <algorithm>
#include <execution>

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
}

vren_demo::ui::ui(vren_demo::app& app) :
	m_app(&app)
{
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	set_ui_style();
}

void vren_demo::ui::set_ui_style()
{
	ImGuiStyle& style = ImGui::GetStyle();

	// light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
	style.Alpha = 1.0f;
	style.FrameRounding = 3.0f;
	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

	for (int i = 0; i <= ImGuiCol_COUNT; i++)
	{
		ImVec4& col = style.Colors[i];
		float H, S, V;
		ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

		if (S < 0.1f)
		{
			V = 1.0f - V;
		}
		ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
		if (col.w < 1.00f)
		{
			col.w *= 0.5f;
		}
	}
}

void vren_demo::ui::show_legend_window()
{
	if (ImGui::Begin("Legend"))
	{
		ImGui::Text("F1 - Switch renderer");
		ImGui::Text("F2 - Hide UI");

		ImGui::Spacing();

		ImGui::Text("1 - Show meshlets");
		ImGui::Text("2 - Show meshlets bounds");
		ImGui::Text("3 - Show projected meshlets bounds");
		ImGui::Text("4 - Show meshlets ID");

		ImGui::Spacing();

		ImGui::Text("W - Forward");
		ImGui::Text("S - Backward");
		ImGui::Text("A - Left");
		ImGui::Text("D - Right");
		ImGui::Text("Space - Up");
		ImGui::Text("Left-shift - Down");
		ImGui::Text("Mouse movement - Rotate");

		ImGui::Spacing();

		ImGui::Text("Enter - Enter camera mode");
		ImGui::Text("Esc - Exit camera mode/close");
	}

	ImGui::End();
}

void vren_demo::ui::show_scene_window()
{
	if (ImGui::Begin("Scene UI##scene_ui", nullptr, NULL))
	{
		// Background color
		ImGui::ColorEdit3("Background color##scene_ui", reinterpret_cast<float*>(&m_app->m_background_color), ImGuiColorEditFlags_Float);

		// Point lights
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Text("Point lights");

		ImGui::Spacing();

		int point_light_count = m_point_light_count;
		
		// Count
		if (ImGui::SliderInt("Count##point_lights-scene_ui", &point_light_count, 0, VREN_MAX_POINT_LIGHT_COUNT, "%d", ImGuiSliderFlags_Logarithmic))
		{
			m_app->m_light_array_fork.enqueue([
				from_count = m_point_light_count,
				to_count = point_light_count,
				color = m_point_light_color,
				intensity = m_point_light_intensity
			](uint32_t idx, vren::light_array& light_array)
			{
				vren::point_light* point_lights = light_array.m_point_light_buffer.get_mapped_pointer<vren::point_light>();
				glm::vec4* point_light_positions = light_array.m_point_light_position_buffer.get_mapped_pointer<glm::vec4>();

				for (uint32_t i = from_count; i < to_count; i++)
				{
					point_lights[i] = {
						.m_color = color,
						.m_intensity = intensity,
					};
					point_light_positions[i] = glm::vec4{ 0, 0, 0, 1 };
				}

				light_array.m_point_light_count = to_count;
			});

			m_point_light_count = point_light_count;
		}

		// Color
		if (ImGui::ColorEdit3("Color##point_lights-scene_ui", reinterpret_cast<float*>(&m_point_light_color), ImGuiColorEditFlags_Float))
		{
			m_app->m_light_array_fork.enqueue([color = m_point_light_color](uint32_t idx, vren::light_array& light_array)
			{
				vren::point_light* point_lights = light_array.m_point_light_buffer.get_mapped_pointer<vren::point_light>();
				std::for_each(std::execution::par, point_lights, point_lights + light_array.m_point_light_count, [&](vren::point_light& point_light)
				{
					point_light.m_color = color;
				});
			});
		}

		// Speed
		ImGui::SliderFloat("Speed##point_lights-scene_ui", &m_app->m_point_light_speed, 0.0f, 0.5f, "%.3f"); // Proportional to model size

		// Intensity
		if (ImGui::SliderFloat("Intensity##point_lights-scene_ui", &m_point_light_intensity, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
		{
			m_app->m_light_array_fork.enqueue([intensity = m_point_light_intensity](uint32_t idx, vren::light_array& light_array)
			{
				vren::point_light* point_lights = light_array.m_point_light_buffer.get_mapped_pointer<vren::point_light>();
				std::for_each(std::execution::par, point_lights, point_lights + light_array.m_point_light_count, [&](vren::point_light& point_light)
				{
					point_light.m_intensity = intensity;
				});
			});
		}

		ImGui::Spacing();

		// Directional light
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Text("Directional light");

		ImGui::Spacing();

		bool changed = false;
		changed |= ImGui::SliderFloat3("Direction##directional_light-scene_ui", reinterpret_cast<float*>(&m_directional_light_direction), -1.0f, 1.0f, "%.1f", 1.0f);
		changed |= ImGui::ColorEdit3("Color##directional_light-scene_ui", reinterpret_cast<float*>(&m_directional_light_color), ImGuiColorEditFlags_Float);

		if (changed)
		{
			m_app->m_light_array_fork.enqueue([](uint32_t idx, vren::light_array& light_array)
			{
				vren::directional_light* directional_lights = light_array.m_directional_light_buffer.get_mapped_pointer<vren::directional_light>();

				directional_lights[0].m_direction = glm::vec3(1.0f, 1.0f, 0.0f);
				directional_lights[0].m_color = glm::vec3(1.0f, 1.0f, 1.0f);

				light_array.m_directional_light_count = 1;
			});
		}

		ImGui::Spacing();
	}

	ImGui::End();
}

void vren_demo::ui::show_profiling_window()
{
	if (ImGui::Begin("Profiling##profiling", nullptr, NULL))
	{
		// General
		ImGui::Text("FPS: %d", m_app->m_fps);
		ImGui::Text("Num. of frame-in-flight: %d", VREN_MAX_FRAME_IN_FLIGHT_COUNT);

		vren::swapchain* swapchain = m_app->m_presenter.get_swapchain();
		if (swapchain) {
			ImGui::Text("Swapchain images: %lld", swapchain->m_images.size());
		}

		// Frame delta
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Text("Frame delta %.3f ms %.3f ms", m_app->m_frame_dt.get_last_value(), m_app->m_frame_dt.get_last_avg());
		
		ImGui::Spacing();

		plot_ui("Frame dt##profiling-general-frame_dt", m_app->m_frame_dt, "ms");
		ImGui::Text("Frame parallelism: %.1f%% %.1f%%", m_app->m_frame_parallelism_pct.get_last_value() * 100, m_app->m_frame_parallelism_pct.get_last_avg() * 100);

		// Profiling slots
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Text("Profiling slots");

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

				char const* slot_name = vren_demo::profile_slot_nmae(static_cast<vren_demo::profile_slot_enum_t>(slot));
				ImGui::Text("%s", slot_name);

				ImGui::TableNextColumn();
				ImGui::Text("%.3f ms", profiled_data.get_last_value() / (1000 * 1000));

				ImGui::TableNextColumn();
				ImGui::Text("%.3f ms", profiled_data.get_last_avg() / (1000 * 1000));
			}

			ImGui::EndTable();
		}
	}

	ImGui::End();
}

void vren_demo::ui::show_controls_window()
{
	if (ImGui::Begin("Controls##controls", nullptr, NULL))
	{
		ImGui::Text("Renderer type: %s", vren_demo::renderer_type_name(m_app->m_selected_renderer_type));

		// General
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Checkbox("UI visible (F2)", &m_app->m_show_ui);

		bool occlusion_culling = m_app->m_mesh_shader_renderer->is_occlusion_culling_enabled();
		if (ImGui::Checkbox("Occlusion culling", &occlusion_culling))
		{
			m_app->m_mesh_shader_renderer = std::make_shared<vren::mesh_shader_renderer>(m_app->m_context, occlusion_culling);
		}

		// Mode clusterization debug
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Checkbox("Show meshlets", &m_app->m_show_meshlets);
		ImGui::Checkbox("Show meshlets bounds", &m_app->m_show_meshlets_bounds);
		ImGui::Checkbox("Show meshlets projected bounds", &m_app->m_show_projected_meshlet_bounds);
		ImGui::Checkbox("Show meshlets ID", &m_app->m_show_instanced_meshlets_indices);

		// Depth-buffer pyramid
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Text("Depth-buffer pyramid");

		ImGui::Checkbox("Show", &m_app->m_show_depth_buffer_pyramid);

		auto& depth_buffer_pyramid = m_app->m_depth_buffer_pyramid;
		uint32_t& depth_buffer_pyramid_level = m_app->m_shown_depth_buffer_pyramid_level;

		ImGui::SliderFloat("Color exponent", &m_app->m_depth_buffer_pyramid_color_exponent, 1.0f, 256.0f);

		ImGui::Checkbox("Draw grid", &m_app->m_depth_buffer_pyramid_draw_grid);

		ImGui::Text("Resolution: (%d, %d)", depth_buffer_pyramid->get_image_width(depth_buffer_pyramid_level), depth_buffer_pyramid->get_image_height(depth_buffer_pyramid_level));
		ImGui::SliderInt("Level", (int32_t*) &depth_buffer_pyramid_level, 0, depth_buffer_pyramid->get_level_count() - 1);

		// Render-graph dump
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		if (ImGui::Button("Take render-graph dump##render_graph_dump"))
		{
			m_app->m_take_next_render_graph_dump = true;
		}

		// Clustered shading
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		ImGui::Text("Clustered shading");
		
		ImGui::Spacing();

		ImGui::Checkbox("Show point light BVH", &m_app->m_show_light_bvh);

		// Show camera clusters
		ImGui::Text("Show clusters geometry");

		if (ImGui::Button("Show##show_clusters_geometry"))
		{
			m_app->m_camera_clusters_draw_buffer = std::make_unique<vren::debug_renderer_draw_buffer>(
				m_app->m_show_clusters_geometry(
					m_app->m_context,
					m_app->m_cluster_and_shade,
					m_app->m_camera,
					m_app->get_screen()
				)
			);
		}

		ImGui::SameLine();
	
		if (ImGui::Button("Clear##show_clusters_geometry"))
		{
			m_app->m_camera_clusters_draw_buffer.reset();
		}

		// Show clusters
		int32_t& show_clusters_mode = m_app->m_show_clusters_mode;

		bool toggled = false;

		int32_t value = show_clusters_mode;
		toggled |= ImGui::RadioButton("Show clusters key", &value, 0) && show_clusters_mode == 0;
		toggled |= ImGui::RadioButton("Show clusters ijk", &value, 1) && show_clusters_mode == 1;
		toggled |= ImGui::RadioButton("Show clusters normal", &value, 2) && show_clusters_mode == 2;
		toggled |= ImGui::RadioButton("Show assigned light counts", &value, 3) && show_clusters_mode == 3;
		toggled |= ImGui::RadioButton("Show assigned light offsets", &value, 4) && show_clusters_mode == 4;
		toggled |= ImGui::RadioButton("Show assigned light indices", &value, 5) && show_clusters_mode == 5;

		show_clusters_mode = toggled ? -1 : value;
	}

	ImGui::End();
}

void vren_demo::ui::show_camera_window()
{
	vren::camera& camera = m_app->m_camera;

	glm::vec4 a(0, 0, 0, 1);
	glm::vec4 b(0, 0, 1, 1);

	if (ImGui::Begin("Camera##camera"))
	{
		ImGui::SliderFloat("Speed", &m_app->m_camera_speed, 0.01f, 10.0f);

		ImGui::Spacing();

		ImGui::Text("Position: %.2f, %.2f, %.2f", camera.m_position.x, camera.m_position.y, camera.m_position.z);
		ImGui::Text("Rotation: %.2f, %.2f", camera.m_yaw, camera.m_pitch);

		ImGui::Spacing();

		ImGui::Text("Forward: (%.2f, %.2f, %.2f)", camera.get_forward().x, camera.get_forward().y, camera.get_forward().z);
		ImGui::Text("Up: (%.2f, %.2f, %.2f)", camera.get_up().x, camera.get_up().y, camera.get_up().z);
		ImGui::Text("Right: (%.2f, %.2f, %.2f)", camera.get_right().x, camera.get_right().y, camera.get_right().z);

		ImGui::Spacing();

		float fov_degrees = glm::degrees(camera.m_fov_y);
		ImGui::SliderFloat("FOV", &fov_degrees, 1, 90, "%.2f deg");
		camera.m_fov_y = glm::radians(fov_degrees);

		ImGui::SliderFloat("Near plane", &camera.m_near_plane, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		ImGui::SliderFloat("Far plane", &camera.m_far_plane, 100.0f, 10000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
		ImGui::Text("Aspect ratio: %.2f", camera.m_aspect_ratio);
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

		ImGuiID remaining_did;
		ImGuiID left_did;
		ImGuiID right_did;
		ImGuiID left_bottom_did, left_top_did;
		ImGuiID right_top_did, right_bottom_did;

		ImGui::DockBuilderSplitNode(m_main_dock_id, ImGuiDir_Left, 0.16f, &left_did, &remaining_did);
		ImGui::DockBuilderSplitNode(remaining_did, ImGuiDir_Right, 0.2f, &right_did, &remaining_did);
		ImGui::DockBuilderSplitNode(left_did, ImGuiDir_Down, 0.5f, &left_bottom_did, &left_top_did);
		ImGui::DockBuilderSplitNode(right_did, ImGuiDir_Down, 0.5f, &right_bottom_did, &right_top_did);

		ImGui::DockBuilderFinish(m_main_dock_id);

		m_controls_window_did = left_top_did;
		m_camera_window_did = left_top_did;
		m_profiling_window_did = left_bottom_did;
		m_legend_window_did = right_top_did;
		m_scene_window_did = right_bottom_did;
	}

	ImGui::DockSpace(m_main_dock_id, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::End();

	ImGui::SetNextWindowDockID(m_legend_window_did, ImGuiCond_Once);
	show_legend_window();

	ImGui::SetNextWindowDockID(m_scene_window_did, ImGuiCond_Once);
	show_scene_window();

	ImGui::SetNextWindowDockID(m_profiling_window_did, ImGuiCond_Once);
	show_profiling_window();

	ImGui::SetNextWindowDockID(m_controls_window_did, ImGuiCond_Once);
	show_controls_window();

	ImGui::SetNextWindowDockID(m_camera_window_did, ImGuiCond_Once);
	show_camera_window();

	//ImGui::ShowDemoWindow();
	//ImPlot::ShowDemoWindow();
}



