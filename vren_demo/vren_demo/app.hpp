#pragma once

#include <volk.h>
#include <GLFW/glfw3.h>

#include <vren/context.hpp>
#include <vren/renderer.hpp>
#include <vren/debug_renderer.hpp>
#include <vren/imgui_renderer.hpp>
#include <vren/depth_buffer_pyramid.hpp>
#include <vren/presenter.hpp>
#include <vren/utils/profiler.hpp>

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

	enum profile_slot_enum_t
	{
		ProfileSlot_NONE = 0, // VREN_MAX_FRAME_IN_FLIGHTS = 4
		ProfileSlot_CLEAR_COLOR_BUFFER = 4,
		ProfileSlot_CLEAR_DEPTH_BUFFER = 8,
		ProfileSlot_BASIC_RENDERER = 12,
		ProfileSlot_MESH_SHADER_RENDERER = 16,
		ProfileSlot_DEBUG_RENDERER = 20,
		ProfileSlot_IMGUI_RENDERER = 24,
		ProfileSlot_BUILD_DEPTH_BUFFER_PYRAMID = 28,
		ProfileSlot_BLIT_COLOR_BUFFER_TO_SWAPCHAIN_IMAGE = 32,

		ProfileSlot_LAST = 35,
		ProfileSlot_Count
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
		vren::debug_renderer_draw_buffer m_debug_draw_buffer;
		vren::debug_renderer_draw_buffer m_meshlets_debug_draw_buffer;
		vren::debug_renderer_draw_buffer m_meshlets_bounds_debug_draw_buffer;

		std::unique_ptr<vren::basic_renderer_draw_buffer> m_basic_renderer_draw_buffer;
		std::unique_ptr<vren::mesh_shader_renderer_draw_buffer> m_mesh_shader_renderer_draw_buffer;

		vren::light_array m_light_array;

		std::vector<vren::vk_utils::color_buffer_t> m_color_buffers;
		std::unique_ptr<vren::vk_utils::depth_buffer_t> m_depth_buffer;
		std::unique_ptr<vren::depth_buffer_pyramid> m_depth_buffer_pyramid;

		vren::depth_buffer_reductor m_depth_buffer_reductor;

		vren::render_graph::allocator m_render_graph_allocator;
		bool m_take_render_graph_dump = false;

		vren::profiler m_profiler;
		std::array<uint64_t, ProfileSlot_Count> m_elapsed_time_by_slot;

		vren_demo::camera m_camera;
		vren_demo::freecam_controller m_freecam_controller;

		vren_demo::tinygltf_parser m_gltf_parser;

		vren_demo::ui m_ui;

		// Behavioral variables
		vren_demo::renderer_type_enum_t m_selected_renderer_type = vren_demo::RendererType_BASIC_RENDERER;
		bool m_show_ui = true;
		bool m_show_meshlets = false;
		bool m_show_meshlets_bounds = false;

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
		void record_commands(uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container);

	public:
		void on_frame();
	};
}
