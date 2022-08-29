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
#include <vren/pipeline/construct_light_array_bvh.hpp>
#include <vren/model/basic_model_draw_buffer.hpp>
#include <vren/model/clusterized_model.hpp>
#include <vren/model/clusterized_model_draw_buffer.hpp>
#include <vren/base/operation_fork.hpp>
#include <vren/pipeline/clustered_shading.hpp>
#include <vren/camera.hpp>

#include "camera_controller.hpp"
#include "point_light_bouncer.hpp"
#include "ui.hpp"
#include "visualize_bvh.hpp"
#include "clustered_shading_debug.hpp"

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

	inline char const* renderer_type_name(renderer_type_enum_t value)
	{
		switch (value)
		{
		case RendererType_NONE:                 return "NONE";
		case RendererType_BASIC_RENDERER:       return "BASIC_RENDERER";
		case RendererType_MESH_SHADER_RENDERER: return "MESH_SHADER_RENDERER";
		default:
			return "?";
		}
	}

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
		ProfileSlot_CONSTRUCT_LIGHT_ARRAY_BVH,

		ProfileSlot_Count
	};

	inline char const* profile_slot_nmae(profile_slot_enum_t profile_slot)
	{
		switch (profile_slot)
		{
		case ProfileSlot_NONE: return "NONE";
		case ProfileSlot_CLEAR_COLOR_BUFFER: return "CLEAR_COLOR_BUFFER";
		case ProfileSlot_CLEAR_DEPTH_BUFFER: return "CLEAR_DEPTH_BUFFER";
		case ProfileSlot_BASIC_RENDERER: return "BASIC_RENDERER";
		case ProfileSlot_MESH_SHADER_RENDERER: return "MESH_SHADER_RENDERER";
		case ProfileSlot_DEBUG_RENDERER: return "DEBUG_RENDERER";
		case ProfileSlot_IMGUI_RENDERER: return "IMGUI_RENDERER";
		case ProfileSlot_BUILD_DEPTH_BUFFER_PYRAMID: return "BUILD_DEPTH_BUFFER_PYRAMID";
		case ProfileSlot_BLIT_COLOR_BUFFER_TO_SWAPCHAIN_IMAGE: return "BLIT_TO_SWAPCHAIN";
		case ProfileSlot_CONSTRUCT_LIGHT_ARRAY_BVH: return "CONSTRUCT_LIGHT_ARRAY_BVH";
		default:
			return "?";
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

		vren_demo::visualize_bvh m_visualize_bvh;

		// Renderers
		vren::basic_renderer m_basic_renderer;
		std::shared_ptr<vren::mesh_shader_renderer> m_mesh_shader_renderer;
		vren::debug_renderer m_debug_renderer;
		vren::imgui_renderer m_imgui_renderer;

		// gBuffer
		std::shared_ptr<vren::gbuffer> m_gbuffer;

		// Presenter
		vren::vk_surface_khr m_surface;
		vren::presenter m_presenter;

		// Debug draw buffers
		vren::debug_renderer_draw_buffer m_debug_draw_buffer; // General purpose
		vren::debug_renderer_draw_buffer m_debug_meshlets_draw_buffer;
		vren::debug_renderer_draw_buffer m_debug_meshlet_bounds_draw_buffer;
		vren::debug_renderer_draw_buffer m_debug_projected_meshlet_bounds_draw_buffer;

		// Model
		glm::vec3 m_model_min = glm::vec3(std::numeric_limits<float>::infinity());
		glm::vec3 m_model_max = glm::vec3(-std::numeric_limits<float>::infinity());
		std::unique_ptr<vren::basic_model_draw_buffer> m_basic_model_draw_buffer;
		std::unique_ptr<vren::clusterized_model> m_clusterized_model;
		std::unique_ptr<vren::clusterized_model_draw_buffer> m_clusterized_model_draw_buffer;

		// Material
		std::array<vren::material_buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_material_buffers;
		vren::operation_fork<vren::material_buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_material_buffer_fork;

		// Lighting
		std::array<vren::light_array, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_light_arrays;
		vren::operation_fork<vren::light_array, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_light_array_fork;

		vren_demo::point_light_bouncer m_point_light_bouncer;
		vren_demo::fill_point_light_debug_draw_buffer m_fill_point_light_debug_draw_buffer;

		// Light array BVH
		vren::vk_utils::buffer m_point_light_bvh_buffer;
		uint32_t m_point_light_bvh_root_index;

		vren::vk_utils::buffer m_point_light_index_buffer;

		vren::debug_renderer_draw_buffer m_point_light_bvh_draw_buffer;

		vren::construct_light_array_bvh m_construct_light_array_bvh;

		// Point light
		vren::vk_utils::buffer m_point_light_direction_buffer;
		std::array<vren::debug_renderer_draw_buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_point_light_debug_draw_buffers;

		float m_point_light_speed = 0.1f;

		// Clustered shading
		vren::cluster_and_shade m_cluster_and_shade;

		vren_demo::visualize_clusters m_visualize_clusters;

		// Color buffer
		std::vector<vren::vk_utils::color_buffer_t> m_color_buffers;

		// Depth buffer
		std::shared_ptr<vren::vk_utils::depth_buffer_t> m_depth_buffer;

		// Depth-buffer pyramid
		std::shared_ptr<vren::depth_buffer_pyramid> m_depth_buffer_pyramid;
		vren::depth_buffer_reductor m_depth_buffer_reductor;

		// Render-graph
		vren::render_graph_allocator m_render_graph_allocator;
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
		vren::camera m_camera;
		float m_camera_speed = 0.1f;

		vren_demo::freecam_controller m_freecam_controller;

		vren_demo::ui m_ui;

		// Behavioral variables
		vren_demo::renderer_type_enum_t m_selected_renderer_type = vren_demo::RendererType_BASIC_RENDERER;
		bool m_show_ui = true;
		bool m_show_meshlets = false;
		bool m_show_meshlets_bounds = false;
		bool m_show_projected_meshlet_bounds = false;
		bool m_show_instanced_meshlets_indices = false;
		bool m_show_depth_buffer_pyramid = false;
		uint32_t m_shown_depth_buffer_pyramid_level = 0;

		bool m_show_light_bvh = false;
		bool m_show_clusters = false;

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
		void record_commands(
			uint32_t frame_idx,
			uint32_t swapchain_image_idx,
			vren::swapchain const& swapchain,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			float dt
		);

	public:
		void on_frame(
			float dt
		);
	};
}
