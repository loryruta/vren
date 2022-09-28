#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include <vren/context.hpp>
#include "vren/pipeline/render_target.hpp"
#include <vren/toolbox.hpp>
#include <vren/vk_helpers/shader.hpp>
#include "vren/pipeline/depth_buffer_pyramid.hpp"

#include "vren/model/tinygltf_parser.hpp"

#define VREN_DEMO_PLOT_SAMPLES_COUNT 512

namespace vren_demo
{
	// Forward decl
	class app;

	// ------------------------------------------------------------------------------------------------
	// UI
	// ------------------------------------------------------------------------------------------------

	class ui
	{
	private:
		vren_demo::app* m_app;

		ImGuiID m_main_dock_id;

		ImGuiID m_controls_window_did;
		ImGuiID m_camera_window_did;
		ImGuiID m_profiling_window_did;
		ImGuiID m_legend_window_did;
		ImGuiID m_scene_window_did;

		int m_point_light_count = 0;
		glm::vec3 m_point_light_color = glm::vec3(1, 1, 1);
		float m_point_light_intensity = 0.01f;

		glm::vec3 m_directional_light_direction{};
		glm::vec3 m_directional_light_color = glm::vec3(1, 1, 0);

	public:
		ui(vren_demo::app& app);

	private:
		void set_ui_style();

		void show_legend_window();
		void show_scene_window();
		void show_controls_window();
		void show_profiling_window();
		void show_camera_window();

	public:
		void show(
			uint32_t frame_idx,
			vren::resource_container& resource_container
		);
	};
}
