#include "app.hpp"

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
	// Resize color buffers
	m_color_buffers.clear();
	for (uint32_t i = 0; i < swapchain.m_images.size(); i++)
	{
		m_color_buffers.push_back(
			vren::vk_utils::create_color_buffer(m_context, swapchain.m_image_width, swapchain.m_image_height, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		);
	}

	// Resize depth buffer
	uint32_t padded_width = vren::round_to_nearest_multiple_of_power_of_2(swapchain.m_image_width, 32); // The depth buffer size must be padded to a multiple of 32 for efficiency reasons
	uint32_t padded_height = vren::round_to_nearest_multiple_of_power_of_2(swapchain.m_image_height, 32);

	m_depth_buffer = std::make_unique<vren::vk_utils::depth_buffer_t>(
		vren::vk_utils::create_depth_buffer(m_context, padded_width, padded_height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	);

	// Resize depth buffer pyramid
	m_depth_buffer_pyramid = std::make_unique<vren::depth_buffer_pyramid>(m_context, padded_width, padded_height);
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
		m_take_render_graph_dump = true;
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

void vren_demo::app::record_commands(uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
{
	// Dump profiling data
	for (uint32_t slot_idx = frame_idx; slot_idx < ProfileSlot_Count; slot_idx += 4) {
		m_elapsed_time_by_slot[slot_idx] = m_profiler.read_elapsed_time(slot_idx);
	}

	vkCmdSetCheckpointNV(command_buffer, "Frame start");

	auto& color_buffer = m_color_buffers.at(frame_idx);

	vren::render_target render_target = vren::render_target::cover(swapchain.m_image_width, swapchain.m_image_height, color_buffer, *m_depth_buffer);

	// Material uploading
	m_context.m_toolbox->m_material_manager.sync_buffer(frame_idx, command_buffer);

	// Light array uploading
	m_light_array.sync_buffers(frame_idx);

	// Render-graph begin
	vren::render_graph::graph_t head, tail;

	// Clear color buffer
	auto clear_color_buffer = vren::clear_color_buffer(m_render_graph_allocator, color_buffer.get_image(), { 0.45f, 0.45f, 0.45f, 0.0f });
	head = m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_CLEAR_COLOR_BUFFER + frame_idx, clear_color_buffer);
	tail = head;

	// Clear depth buffer
	auto clear_depth_buffer = vren::clear_depth_stencil_buffer(m_render_graph_allocator, m_depth_buffer->get_image(), { .depth = 1.0f, .stencil = 0 });
	tail = vren::render_graph::concat(
		m_render_graph_allocator,
		tail,
		m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_CLEAR_DEPTH_BUFFER + frame_idx, clear_depth_buffer)
	);

	// Render scene
	switch (m_selected_renderer_type)
	{
	case vren_demo::RendererType_BASIC_RENDERER:
		if (m_basic_renderer_draw_buffer)
		{
			auto basic_render = m_basic_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_light_array, *m_basic_renderer_draw_buffer);
			tail = vren::render_graph::concat(
				m_render_graph_allocator,
				tail,
				m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_BASIC_RENDERER + frame_idx, basic_render)
			);
		}
		break;
	case vren_demo::RendererType_MESH_SHADER_RENDERER:
		if (m_mesh_shader_renderer_draw_buffer)
		{
			auto mesh_shader_render = m_mesh_shader_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_light_array, *m_mesh_shader_renderer_draw_buffer);
			tail = vren::render_graph::concat(
				m_render_graph_allocator,
				tail,
				m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_MESH_SHADER_RENDERER + frame_idx, mesh_shader_render)
			);
		}
		break;
	default:
		break;
	}

	// Build depth buffer pyramid
	auto build_depth_buffer_pyramid = m_depth_buffer_reductor.copy_and_reduce(m_render_graph_allocator, *m_depth_buffer, *m_depth_buffer_pyramid);
	tail = vren::render_graph::concat(
		m_render_graph_allocator,
		tail,
		m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_BUILD_DEPTH_BUFFER_PYRAMID + frame_idx, build_depth_buffer_pyramid)
	);

	// Debug general purpose objects
	vren::render_graph::graph_t debug_head, debug_tail;

	debug_head = m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_debug_draw_buffer);
	debug_tail = debug_head;

	// Debug meshlets
	if (m_show_meshlets)
	{
		debug_tail = vren::render_graph::concat(
			m_render_graph_allocator,
			debug_tail,
			m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_meshlets_debug_draw_buffer)
		);
	}

	// Debug meshlet bounds
	if (m_show_meshlets_bounds)
	{
		debug_tail = vren::render_graph::concat(
			m_render_graph_allocator,
			debug_tail,
			m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), m_meshlets_bounds_debug_draw_buffer)
		);
	}

	// Debug depth buffer pyramid
	if (m_ui.m_depth_buffer_pyramid_ui.m_show)
	{
		debug_tail = vren::render_graph::concat(
			m_render_graph_allocator,
			debug_tail,
			vren::blit_depth_buffer_pyramid_level_to_color_buffer(m_render_graph_allocator, *m_depth_buffer_pyramid, m_ui.m_depth_buffer_pyramid_ui.m_selected_level, color_buffer, swapchain.m_image_width, swapchain.m_image_height)
		);
	}

	tail = vren::render_graph::concat(
		m_render_graph_allocator,
		tail,
		m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_DEBUG_RENDERER + frame_idx, debug_head)
	);

	// Render UI
	if (m_show_ui)
	{
		auto show_ui = [&]() {
			m_ui.show(*m_depth_buffer_pyramid, m_light_array);
		};

		auto imgui_render_node = m_imgui_renderer.render(m_render_graph_allocator, render_target, show_ui);
		tail = vren::render_graph::concat(
			m_render_graph_allocator,
			tail,
			m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_IMGUI_RENDERER + frame_idx, imgui_render_node)
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
	tail = vren::render_graph::concat(
		m_render_graph_allocator,
		tail,
		m_profiler.profile(m_render_graph_allocator, vren_demo::ProfileSlot_BLIT_COLOR_BUFFER_TO_SWAPCHAIN_IMAGE + frame_idx, blit_to_swapchain_node)
	);

	// Ensure swapchain image is in "present" layout
	tail = vren::render_graph::concat(
		m_render_graph_allocator,
		tail,
		vren::transit_swapchain_image_to_present_layout(m_render_graph_allocator, swapchain.m_images.at(swapchain_image_idx))
	);

	(void) tail;

	// Execute render-graph
	vren::render_graph::execute(m_render_graph_allocator, head, frame_idx, command_buffer, resource_container);

	if (m_take_render_graph_dump)
	{
		std::filesystem::path render_graph_dump_filename(
			fmt::format("{}_render_graph.dot", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())
		);
		vren::render_graph::dump(m_render_graph_allocator, head, render_graph_dump_filename);

		m_take_render_graph_dump = false;
	}

	m_render_graph_allocator.clear();
}

void vren_demo::app::on_frame()
{
	m_presenter.present([&](uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		record_commands(frame_idx, swapchain_image_idx, swapchain, command_buffer, resource_container);
	});
}
