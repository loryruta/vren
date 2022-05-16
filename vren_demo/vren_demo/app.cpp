#include "app.hpp"

#include <fstream>

#include <imgui_impl_glfw.h>

#include "scene/scene_gpu_uploader.hpp"

vren_demo::app::app(GLFWwindow* window) :
	m_window(window),

	m_context([]() {
		vren::context_info context_info{
			.m_app_name = "vren_demo",
			.m_app_version = VK_MAKE_VERSION(1, 0, 0),
			.m_layers = {},
			.m_extensions = {},
			.m_device_extensions = {}
		};

		// Add GLFW extensions
		uint32_t glfw_extension_count = 0;
		char const** glfw_extensions;
		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		context_info.m_extensions.insert(context_info.m_extensions.end(), glfw_extensions, glfw_extensions + glfw_extension_count);

		return vren::context(context_info);
	}()),

	// Renderers
	m_basic_renderer(m_context),
	m_mesh_shader_renderer(m_context),
	m_debug_renderer(m_context),
	m_imgui_renderer(m_context, vren::imgui_windowing_backend_hooks{
		.m_init_callback      = [window]() { ImGui_ImplGlfw_InitForVulkan(window, true); },
		.m_new_frame_callback = []() { ImGui_ImplGlfw_NewFrame(); },
		.m_shutdown_callback  = []() { ImGui_ImplGlfw_Shutdown(); }
	}),

	// Presenter
	m_surface([this]() {
		VkSurfaceKHR surface_handle;
		VREN_CHECK(glfwCreateWindowSurface(m_context.m_instance, m_window, nullptr, &surface_handle), &m_context);
		return vren::vk_surface_khr(m_context, surface_handle);
	}()),
	m_presenter(m_context, m_surface, [this](vren::swapchain const& swapchain) {
		on_swapchain_change(swapchain);
	}),

	// Debug draw buffers
	m_debug_draw_buffer(m_context),
	m_meshlets_debug_draw_buffer(m_context),
	m_meshlets_bounds_debug_draw_buffer(m_context),

	m_light_array(m_context),

	m_color_buffers{},
	m_depth_buffer{},
	m_depth_buffer_pyramid{},

	m_depth_buffer_reductor(m_context),

	// Profiler
	m_profiler(m_context),

	m_gltf_parser(m_context),

	m_camera{},
	m_freecam_controller(m_window),

	m_ui(*this)
{
	late_initialize();
}

vren_demo::app::~app()
{
	VREN_CHECK(vkDeviceWaitIdle(m_context.m_device), &m_context);
}

void vren_demo::app::late_initialize()
{
	glfwSetWindowUserPointer(m_window, this);

	// Key callback binding
	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto app = static_cast<vren_demo::app*>(glfwGetWindowUserPointer(window));
		app->on_key_press(key, scancode, action, mods);
	});

	// Draw cartesian axes
	m_debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(1, 0, 0), .m_color = 0xff0000 });
	m_debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 1, 0), .m_color = 0x00ff00 });
	m_debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 0, 1), .m_color = 0x0000ff });
}

void vren_demo::app::on_swapchain_change(vren::swapchain const& swapchain)
{
	uint32_t width = swapchain.m_image_width;
	uint32_t height = swapchain.m_image_height;

	// Resize color buffers
	m_color_buffers.clear();
	for (uint32_t i = 0; i < VREN_MAX_FRAME_IN_FLIGHT_COUNT; i++)
	{
		m_color_buffers.push_back(
			vren::vk_utils::create_color_buffer(m_context, width, height, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		);
	}

	// Resize depth buffer
	m_depth_buffer = std::make_unique<vren::vk_utils::depth_buffer_t>(
		vren::vk_utils::create_depth_buffer(m_context, width, height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	);

	// Resize depth buffer pyramid
	m_depth_buffer_pyramid = std::make_unique<vren::depth_buffer_pyramid>(m_context, width, height);
}

void vren_demo::app::on_key_press(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
	{
		m_show_meshlets = !m_show_meshlets;
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
	{
		m_show_meshlets_bounds = !m_show_meshlets_bounds;
	}

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
	{
		m_selected_renderer_type = static_cast<vren_demo::renderer_type_enum_t>((m_selected_renderer_type + 1) % vren_demo::RendererType_Count);
	}

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
	{
		m_show_ui = !m_show_ui;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		m_camera.m_position = glm::vec3(0);
	}

	if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
	{
		m_take_next_render_graph_dump = true;
	}

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				glfwSetWindowShouldClose(m_window, true);
			}
			break;
		case GLFW_KEY_ENTER:
			if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			break;
		}
	}
}

vren::mesh_shader_renderer_draw_buffer upload_scene_for_mesh_shader_renderer_(vren::context const& context, vren_demo::intermediate_scene const& parsed_scene, vren::debug_renderer_draw_buffer& meshlet_debug_draw_buffer, vren::debug_renderer_draw_buffer& meshlet_bounds_debug_draw_buffer)
{
	// Allocation
	size_t
		meshlet_vertex_buffer_size,
		meshlet_triangle_buffer_size,
		meshlet_buffer_size,
		instanced_meshlet_buffer_size;

	vren_demo::get_clusterized_scene_requested_buffer_sizes(
		parsed_scene.m_vertices.data(),
		parsed_scene.m_indices.data(),
		parsed_scene.m_instances.data(),
		parsed_scene.m_meshes.data(),
		parsed_scene.m_meshes.size(),
		meshlet_vertex_buffer_size,
		meshlet_triangle_buffer_size,
		meshlet_buffer_size,
		instanced_meshlet_buffer_size
	);

	// Clustering
	std::vector<uint32_t> meshlet_vertices(meshlet_vertex_buffer_size);
	std::vector<uint8_t> meshlet_triangles(meshlet_triangle_buffer_size);
	std::vector<vren::meshlet> meshlets(meshlet_buffer_size);
	std::vector<vren::instanced_meshlet> instanced_meshlets(instanced_meshlet_buffer_size);
	size_t
		meshlet_vertex_count,
		meshlet_triangle_count,
		meshlet_count,
		instanced_meshlet_count;

	vren_demo::clusterize_scene(
		parsed_scene.m_vertices.data(),
		parsed_scene.m_indices.data(),
		parsed_scene.m_instances.data(),
		parsed_scene.m_meshes.data(),
		parsed_scene.m_meshes.size(),
		meshlet_vertices.data(),
		meshlet_vertex_count,
		meshlet_triangles.data(),
		meshlet_triangle_count,
		meshlets.data(),
		meshlet_count,
		instanced_meshlets.data(),
		instanced_meshlet_count
	);

	meshlet_vertices.resize(meshlet_vertex_count);
	meshlet_triangles.resize(meshlet_triangle_count);
	meshlets.resize(meshlet_count);
	instanced_meshlets.resize(instanced_meshlet_count);

	// Debug for meshlet geometry
	std::vector<vren::debug_renderer_line> debug_lines;
	vren_demo::write_debug_information_for_meshlet_geometry(
		parsed_scene.m_vertices.data(),
		meshlet_vertices.data(),
		meshlet_triangles.data(),
		meshlets.data(),
		instanced_meshlets.data(),
		instanced_meshlet_count,
		parsed_scene.m_instances.data(),
		debug_lines
	);
	meshlet_debug_draw_buffer.clear();
	meshlet_debug_draw_buffer.add_lines(debug_lines.data(), debug_lines.size());

	// Debug for meshlet bounds
	std::vector<vren::debug_renderer_sphere> debug_spheres;
	vren_demo::write_debug_information_for_meshlet_bounds(
		meshlets.data(),
		instanced_meshlets.data(),
		instanced_meshlet_count,
		parsed_scene.m_instances.data(),
		debug_spheres
	);
	meshlet_bounds_debug_draw_buffer.clear();
	meshlet_bounds_debug_draw_buffer.add_spheres(debug_spheres.data(), debug_spheres.size());

	// Uploading
	auto draw_buffer = vren_demo::upload_scene_for_mesh_shader_renderer(
		context,
		parsed_scene.m_vertices.data(),
		parsed_scene.m_vertices.size(),
		meshlet_vertices.data(),
		meshlet_vertices.size(),
		meshlet_triangles.data(),
		meshlet_triangles.size(),
		meshlets.data(),
		meshlet_count,
		instanced_meshlets.data(),
		instanced_meshlet_count,
		parsed_scene.m_instances.data(),
		parsed_scene.m_instances.size()
	);
	return std::move(draw_buffer);
}

void vren_demo::app::load_scene(char const* gltf_model_filename)
{
	vren_demo::intermediate_scene parsed_scene;

	vren_demo::tinygltf_parser gltf_parser(m_context);
	gltf_parser.load_from_file(gltf_model_filename, parsed_scene);

	m_basic_renderer_draw_buffer = std::make_unique<vren::basic_renderer_draw_buffer>(
		vren_demo::upload_scene_for_basic_renderer(m_context, parsed_scene)
	);

	m_mesh_shader_renderer_draw_buffer = std::make_unique<vren::mesh_shader_renderer_draw_buffer>(
		upload_scene_for_mesh_shader_renderer_(m_context, parsed_scene, m_meshlets_debug_draw_buffer, m_meshlets_bounds_debug_draw_buffer)
	);
}

void vren_demo::app::on_window_resize(uint32_t width, uint32_t height)
{
	m_presenter.recreate_swapchain(width, height);
}

void vren_demo::app::on_update(float dt)
{
	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(m_window, &framebuffer_width, &framebuffer_height);

	m_camera.m_aspect_ratio = framebuffer_width / (float) framebuffer_height;

	m_freecam_controller.update(m_camera, dt, 0.1f, glm::radians(90.0f));
}

void vren_demo::app::record_commands(
	uint32_t frame_idx,
	uint32_t swapchain_image_idx,
	vren::swapchain const& swapchain,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container
)
{
	// Render-graph begin
	vren::render_graph::chain render_graph(m_render_graph_allocator);

	// Dump profiling data
	for (uint32_t slot_idx = 0; slot_idx < ProfileSlot_Count; slot_idx++)
	{
		uint64_t time_elapsed = m_profiler.read_elapsed_time(slot_idx, frame_idx);
		if (time_elapsed != UINT64_MAX) {
			m_delta_time_by_profile_slot[slot_idx].push_value(time_elapsed);
		}
	}

	vkCmdSetCheckpointNV(command_buffer, "Frame start");

	auto& color_buffer = m_color_buffers.at(frame_idx);

	auto render_target = vren::render_target::cover(swapchain.m_image_width, swapchain.m_image_height, color_buffer, *m_depth_buffer);

	// Material uploading
	auto sync_material_buffer = m_context.m_toolbox->m_material_manager.sync_buffer(m_render_graph_allocator, frame_idx);
	render_graph.concat(sync_material_buffer);

	// Light array uploading
	auto sync_light_buffers = m_light_array.sync_buffers(m_render_graph_allocator, frame_idx);
	render_graph.concat(sync_light_buffers);

	// Clear color buffer
	auto clear_color_buffer = vren::clear_color_buffer(m_render_graph_allocator, color_buffer.get_image(), { 0.45f, 0.45f, 0.45f, 0.0f });
	render_graph.concat(m_profiler.profile(m_render_graph_allocator, clear_color_buffer, vren_demo::ProfileSlot_CLEAR_COLOR_BUFFER, frame_idx));

	// Clear depth buffer
	auto clear_depth_buffer = vren::clear_depth_stencil_buffer(m_render_graph_allocator, m_depth_buffer->get_image(), { .depth = 1.0f, .stencil = 0 });
	render_graph.concat(m_profiler.profile(m_render_graph_allocator, clear_depth_buffer, vren_demo::ProfileSlot_CLEAR_DEPTH_BUFFER, frame_idx));

	// Render scene
	switch (m_selected_renderer_type)
	{
	case vren_demo::RendererType_BASIC_RENDERER:
		if (m_basic_renderer_draw_buffer)
		{
			auto basic_render = m_basic_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_light_array, *m_basic_renderer_draw_buffer);
			render_graph.concat(m_profiler.profile(m_render_graph_allocator, basic_render, vren_demo::ProfileSlot_BASIC_RENDERER, frame_idx));
		}
		break;
	case vren_demo::RendererType_MESH_SHADER_RENDERER:
		if (m_mesh_shader_renderer_draw_buffer)
		{
			auto mesh_shader_render = m_mesh_shader_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_light_array, *m_mesh_shader_renderer_draw_buffer, *m_depth_buffer_pyramid);
			render_graph.concat(m_profiler.profile(m_render_graph_allocator, mesh_shader_render, vren_demo::ProfileSlot_MESH_SHADER_RENDERER, frame_idx));
		}
		break;
	default:
		break;
	}

	// Build depth buffer pyramid
	auto build_depth_buffer_pyramid = m_depth_buffer_reductor.copy_and_reduce(m_render_graph_allocator, *m_depth_buffer, *m_depth_buffer_pyramid);
	render_graph.concat(build_depth_buffer_pyramid);

	// Debug general purpose objects
	vren::render_graph::chain debug_render_graph(m_render_graph_allocator);

	debug_render_graph.concat(m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_debug_draw_buffer));

	// Debug meshlets
	if (m_show_meshlets)
	{
		debug_render_graph.concat(m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_meshlets_debug_draw_buffer));
	}

	// Debug meshlet bounds
	if (m_show_meshlets_bounds)
	{
		debug_render_graph.concat(m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_meshlets_bounds_debug_draw_buffer));
	}

	// Debug depth buffer pyramid
	if (m_ui.m_depth_buffer_pyramid_ui.m_show)
	{
		debug_render_graph.concat(
			vren::blit_depth_buffer_pyramid_level_to_color_buffer(
				m_render_graph_allocator,
				*m_depth_buffer_pyramid,
				m_ui.m_depth_buffer_pyramid_ui.m_selected_level,
				color_buffer,
				swapchain.m_image_width, swapchain.m_image_height
			)
		);
	}

	render_graph.concat(
		m_profiler.profile(m_render_graph_allocator, debug_render_graph.m_head, vren_demo::ProfileSlot_DEBUG_RENDERER, frame_idx)
	);

	// Render UI
	if (m_show_ui)
	{
		auto show_ui = [&]()
		{
			m_ui.show(frame_idx, resource_container);
		};

		auto imgui_render_node = m_imgui_renderer.render(m_render_graph_allocator, render_target, show_ui);
		render_graph.concat(
			m_profiler.profile(m_render_graph_allocator, imgui_render_node, vren_demo::ProfileSlot_IMGUI_RENDERER, frame_idx)
		);
	}

	// Blit to swapchain image
	auto blit_to_swapchain_node = vren::blit_color_buffer_to_swapchain_image(
		m_render_graph_allocator,
		color_buffer,
		swapchain.m_image_width, swapchain.m_image_height,
		swapchain.m_images.at(swapchain_image_idx),
		swapchain.m_image_width, swapchain.m_image_height
	);
	render_graph.concat(
		m_profiler.profile(m_render_graph_allocator, blit_to_swapchain_node, vren_demo::ProfileSlot_BLIT_COLOR_BUFFER_TO_SWAPCHAIN_IMAGE, frame_idx)
	);

	// Ensure swapchain image is in "present" layout
	render_graph.concat(
		vren::transit_swapchain_image_to_present_layout(m_render_graph_allocator, swapchain.m_images.at(swapchain_image_idx))
	);

	// Execute render-graph
	vren::render_graph::execute(m_render_graph_allocator, render_graph.m_head, frame_idx, command_buffer, resource_container);

	if (m_take_next_render_graph_dump)
	{
		std::ofstream output_file(m_render_graph_dump_file);
		vren::render_graph::dump(m_render_graph_allocator, render_graph.m_head, output_file);

		m_take_next_render_graph_dump = false;
	}

	m_render_graph_allocator.clear();
}

float vren_demo::app::calc_frame_parallelism_percentage(uint32_t frame_idx)
{
	// The following algorithm is used to take the temporal interval while one frame was working in parallel
	// with other frames. As a requirement for this algorithm to work, all frames starting timestamp from
	// the oldest frame to the newest *MUST BE SEQUENTIAL*

	int start_fi = (frame_idx + 1) % VREN_MAX_FRAME_IN_FLIGHT_COUNT; // Oldest frame
	int fi = start_fi;

	uint64_t left_t = m_frame_started_at[start_fi];
	uint64_t right_t = 0;

	uint64_t tot_par_dt = 0;

	while (true)
	{
		uint64_t slice_start_t = m_frame_started_at[fi];
		uint64_t slice_end_t = m_frame_ended_at[fi];

		for (int next_fi = (fi + 1) % VREN_MAX_FRAME_IN_FLIGHT_COUNT; next_fi != fi; next_fi = (next_fi + 1) % VREN_MAX_FRAME_IN_FLIGHT_COUNT)
		{
			slice_start_t = std::max(std::min(m_frame_started_at[next_fi], slice_start_t), m_frame_started_at[fi]);
			slice_end_t = std::min(std::max(m_frame_ended_at[next_fi], slice_end_t), m_frame_ended_at[fi]);
		}

		slice_start_t = std::max(right_t, slice_start_t);
		slice_end_t = std::max(right_t, slice_end_t);
		right_t = slice_end_t;

		if (slice_end_t > slice_start_t) {
			tot_par_dt += slice_end_t - slice_start_t;
		}

		fi = (fi + 1) % VREN_MAX_FRAME_IN_FLIGHT_COUNT;
		if (fi == start_fi) {
			break;
		}
	}

	if (right_t > left_t)
	{
		return ((float) tot_par_dt) / (float) (right_t - left_t);
	}
	else
	{
		return 0;
	}
}

void vren_demo::app::on_frame()
{
	m_presenter.present([&](uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		m_frame_ended_at[frame_idx] =
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		m_frame_parallelism_pct.push_value(calc_frame_parallelism_percentage(frame_idx));

		uint64_t frame_dt = m_frame_ended_at[frame_idx] - m_frame_started_at[frame_idx];
		m_frame_dt.push_value(double(frame_dt) / (1000.0 * 1000.0) /* frame_dt (in ms) */);

		m_frame_started_at[frame_idx] =
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		m_fps_counter++;

		double now = glfwGetTime();
		if (m_last_fps_time < 0.0 || now - m_last_fps_time >= 1.0)
		{
			m_fps = m_fps_counter;
			m_fps_counter = 0;
			m_last_fps_time = now;
		}

		record_commands(frame_idx, swapchain_image_idx, swapchain, command_buffer, resource_container);
	});
}