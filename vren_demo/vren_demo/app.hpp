#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vren/context.hpp>
#include "vren/pipeline/basic_renderer.hpp"
#include "vren/pipeline/mesh_shader_renderer.hpp"
#include "vren/pipeline/debug_renderer.hpp"
#include "vren/pipeline/imgui_renderer.hpp"
#include "vren/pipeline/depth_buffer_pyramid.hpp"
#include <vren/presenter.hpp>
#include "vren/pipeline/profiler.hpp"
#include <vren/model/basic_model_draw_buffer.hpp>
#include <vren/model/clusterized_model.hpp>
#include <vren/model/clusterized_model_draw_buffer.hpp>

#include "camera.hpp"
#include "ui.hpp"

namespace vren_demo
{
	// ------------------------------------------------------------------------------------------------

	enum renderer_type_enum_t
	{
		RendererType_NONE,
		RendererType_BASIC_RENDERER,
		RendererType_MESH_SHADER_RENDERER,

		RendererType_Count
	};

	// ------------------------------------------------------------------------------------------------
	// Profiling
	// ------------------------------------------------------------------------------------------------


	enum profile_slot_enum_t
	{
		ProfileSlot_NONE = 0,
		ProfileSlot_CLEAR_COLOR_BUFFER,
		ProfileSlot_CLEAR_DEPTH_BUFFER,
		ProfileSlot_BASIC_RENDERER,
		ProfileSlot_MESH_SHADER_RENDERER,
		ProfileSlot_DEBUG_RENDERER,
		ProfileSlot_IMGUI_RENDERER,
		ProfileSlot_BUILD_DEPTH_BUFFER_PYRAMID,
		ProfileSlot_BLIT_COLOR_BUFFER_TO_SWAPCHAIN_IMAGE,

		ProfileSlot_Count
	};

	inline char const* get_profile_slot_name(profile_slot_enum_t profile_slot)
	{
		switch (profile_slot)
		{
		case ProfileSlot_NONE: return "None";
		case ProfileSlot_CLEAR_COLOR_BUFFER: return "Clear color-buffer";
		case ProfileSlot_CLEAR_DEPTH_BUFFER: return "Clear depth-buffer";
		case ProfileSlot_BASIC_RENDERER: return "Basic renderer";
		case ProfileSlot_MESH_SHADER_RENDERER: return "Mesh shader renderer";
		case ProfileSlot_DEBUG_RENDERER: return "Debug renderer";
		case ProfileSlot_IMGUI_RENDERER: return "ImGui renderer";
		case ProfileSlot_BUILD_DEPTH_BUFFER_PYRAMID: return "Build depth-buffer pyramid";
		case ProfileSlot_BLIT_COLOR_BUFFER_TO_SWAPCHAIN_IMAGE: return "Blit to swapchain";
		default: return "Unknown";
		}
	}

	template<size_t _sample_count>
	struct profiled_data
	{
		std::array<double, _sample_count> m_values{};
		std::array<double, _sample_count> m_avg{};
		double m_sum = 0;

		inline float get_last_value() const
		{
			return m_values[_sample_count - 1];
		}

		inline float get_last_avg() const
		{
			return m_avg[_sample_count - 1];
		}

		inline void push_value(double value)
		{
			m_sum -= m_values[0];
			m_sum += value;

			for (uint32_t i = 0; i < _sample_count - 1; i++) {
				m_values[i] = m_values[i + 1];
			}

			m_values[_sample_count - 1] = value;

			for (uint32_t i = 0; i < _sample_count - 1; i++) {
				m_avg[i] = m_avg[i + 1];
			}

			m_avg[_sample_count - 1] = m_sum / double(_sample_count);
		}
	};

	// ------------------------------------------------------------------------------------------------
	// App
	// ------------------------------------------------------------------------------------------------

	class app
	{
		friend vren_demo::ui;

	private:
		GLFWwindow* m_window;

		vren::context m_context;

		// Renderers
		vren::basic_renderer m_basic_renderer;
		vren::mesh_shader_renderer m_mesh_shader_renderer;
		vren::debug_renderer m_debug_renderer;
		vren::imgui_renderer m_imgui_renderer;

		// Presenter
		vren::vk_surface_khr m_surface;
		vren::presenter m_presenter;

		// Debug draw buffers
		vren::debug_renderer_draw_buffer m_debug_draw_buffer; // General purpose
		vren::debug_renderer_draw_buffer m_debug_meshlets_draw_buffer;
		vren::debug_renderer_draw_buffer m_debug_meshlet_bounds_draw_buffer;
		vren::debug_renderer_draw_buffer m_debug_projected_meshlet_bounds_draw_buffer;

		// Model
		std::unique_ptr<vren::basic_model_draw_buffer> m_basic_model_draw_buffer;
		std::unique_ptr<vren::clusterized_model> m_clusterized_model;
		std::unique_ptr<vren::clusterized_model_draw_buffer> m_clusterized_model_draw_buffer;

		vren::light_array m_light_array;

		std::vector<vren::vk_utils::color_buffer_t> m_color_buffers;
		std::unique_ptr<vren::vk_utils::depth_buffer_t> m_depth_buffer;

		// Depth-buffer pyramid
		std::unique_ptr<vren::depth_buffer_pyramid> m_depth_buffer_pyramid;
		vren::depth_buffer_reductor m_depth_buffer_reductor;

		// Render-graph
		vren::render_graph::allocator m_render_graph_allocator;
		char m_render_graph_dump_file[256] = "render_graph.dot";
		bool m_take_next_render_graph_dump = false;

		// Profiling
		vren::profiler m_profiler;

		double m_last_fps_time = -1.0;
		uint32_t m_fps_counter = 0;
		uint32_t m_fps = 0;

		std::array<uint64_t, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_frame_started_at{};
		std::array<uint64_t, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_frame_ended_at{};
		vren_demo::profiled_data<720> m_frame_dt{};
		vren_demo::profiled_data<32> m_frame_parallelism_pct{};

		std::array<vren_demo::profiled_data<32>, ProfileSlot_Count> m_delta_time_by_profile_slot{};

		// Camera
		vren_demo::camera m_camera;
		vren_demo::freecam_controller m_freecam_controller;

		vren_demo::ui m_ui;

		// Behavioral variables
		vren_demo::renderer_type_enum_t m_selected_renderer_type = vren_demo::RendererType_BASIC_RENDERER;
		bool m_show_ui = true;
		bool m_show_meshlets = false;
		bool m_show_meshlets_bounds = false;
		bool m_show_projected_meshlet_bounds = false;
		bool m_show_instanced_meshlets_indices = false;

	public:
		app(GLFWwindow* window);
		~app();

	private:
		void late_initialize();

		void on_swapchain_change(vren::swapchain const& swapchain);

		void on_key_press(int key, int scancode, int action, int mods);

	public:
		void load_scene(char const* gltf_model_filename);

		void on_window_resize(uint32_t width, uint32_t height);
		void on_update(float dt);

	private:
		float calc_frame_parallelism_percentage(uint32_t frame_idx);

	private:
		void record_commands(uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container);

	public:
		void on_frame();
	};
}
