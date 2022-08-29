#include "app.hpp"

#include <fstream>

#include <imgui_impl_glfw.h>

#include <vren/base/base.hpp>
#include <vren/model/basic_model_uploader.hpp>
#include <vren/model/model_clusterizer.hpp>
#include <vren/model/clusterized_model_uploader.hpp>
#include <vren/pipeline/imgui_utils.hpp>

#include <glm/gtc/random.hpp>

#include "clusterized_model_debugger.hpp"

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
	m_mesh_shader_renderer(std::make_shared<vren::mesh_shader_renderer>(m_context, true)),
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
	m_debug_meshlets_draw_buffer(m_context),
	m_debug_meshlet_bounds_draw_buffer(m_context),
	m_debug_projected_meshlet_bounds_draw_buffer(m_context),

	// Material
	m_material_buffers(
		vren::create_array<vren::material_buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&](uint32_t index) {
			return vren::material_buffer(m_context);
		})
	),

	// Lighting
	m_light_arrays(
		vren::create_array<vren::light_array, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&](uint32_t index) {
			return vren::light_array(m_context);
		})
	),

	m_point_light_bouncer(m_context),
	m_fill_point_light_debug_draw_buffer(m_context),

	m_visualize_bvh(m_context),

	// Light array BVH
	m_point_light_bvh_buffer([&]()
	{
		vren::vk_utils::buffer buffer = vren::vk_utils::alloc_device_only_buffer(
			m_context,
			vren::construct_light_array_bvh::get_required_bvh_buffer_usage_flags(),
			vren::construct_light_array_bvh::get_required_bvh_buffer_size(VREN_MAX_POINT_LIGHT_COUNT)
		);

		vren::vk_utils::set_name(m_context, buffer, "light_bvh_buffer");

		return buffer;
	}()),
	m_point_light_index_buffer([&]()
	{
		vren::vk_utils::buffer buffer = vren::vk_utils::alloc_device_only_buffer(
			m_context,
			vren::construct_light_array_bvh::get_required_light_index_buffer_usage_flags(),
			vren::construct_light_array_bvh::get_required_light_index_buffer_size(VREN_MAX_POINT_LIGHT_COUNT)
		);

		vren::vk_utils::set_name(m_context, buffer, "light_index_buffer");

		return buffer;
	}()),
	m_point_light_bvh_draw_buffer([&]()
	{
		vren::debug_renderer_draw_buffer draw_buffer(m_context);
		draw_buffer.m_vertex_count = 0;
		draw_buffer.m_vertex_buffer.set_data(nullptr, vren::calc_bvh_buffer_length(VREN_MAX_POINT_LIGHT_COUNT) * 12 * sizeof(vren::debug_renderer_vertex));
		return draw_buffer;
	}()),
	m_construct_light_array_bvh(m_context),

	// Point lights
	m_point_light_direction_buffer([&]()
	{
		std::vector<glm::vec4> point_light_directions(VREN_MAX_POINT_LIGHT_COUNT);
		for (glm::vec4& direction : point_light_directions)
		{
			direction = glm::vec4(glm::ballRand(1.0f), 0.0f);
		}

		return vren::vk_utils::create_device_only_buffer(m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, point_light_directions.data(), point_light_directions.size() * sizeof(glm::vec4));
	}()),
	m_point_light_debug_draw_buffers(vren::create_array<vren::debug_renderer_draw_buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&](uint32_t index)
	{
		vren::debug_renderer_draw_buffer draw_buffer(m_context);

		// The vertex count will be later updated based on the number of point lights, while the storage is statically allocated
		draw_buffer.m_vertex_count = 0;
		draw_buffer.m_vertex_buffer.set_data(nullptr, VREN_MAX_POINT_LIGHT_COUNT * 6 * sizeof(vren::debug_renderer_vertex));

		return draw_buffer;
	})),

	// Clustered shading
	m_cluster_and_shade(m_context),

	m_visualize_clusters(m_context),

	// Output
	m_color_buffers{},
	m_depth_buffer{},
	m_depth_buffer_pyramid{},

	m_depth_buffer_reductor(m_context),

	// Profiler
	m_profiler(m_context),

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

	// Init directional light
	m_light_array_fork.enqueue([](vren::light_array& light_array)
	{
		vren::directional_light* directional_lights = light_array.m_directional_light_buffer.get_mapped_pointer<vren::directional_light>();

		directional_lights[0].m_direction = glm::vec3(1.0f, 1.0f, 0.0f);
		directional_lights[0].m_color = glm::vec3(1.0f, 1.0f, 1.0f);

		light_array.m_directional_light_count = 1;
	});

}

void vren_demo::app::on_swapchain_change(vren::swapchain const& swapchain)
{
	uint32_t width = swapchain.m_image_width;
	uint32_t height = swapchain.m_image_height;

	m_color_buffers.clear();
	for (uint32_t i = 0; i < VREN_MAX_FRAME_IN_FLIGHT_COUNT; i++)
	{
		m_color_buffers.push_back(
			vren::vk_utils::create_color_buffer(
				m_context,
				width, height,
				VREN_COLOR_BUFFER_OUTPUT_FORMAT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT
			)
		);
	}

	m_depth_buffer = std::make_shared<vren::vk_utils::depth_buffer_t>(
		vren::vk_utils::create_depth_buffer(
			m_context,
			width, height,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
		)
	);
	m_gbuffer = std::make_shared<vren::gbuffer>(m_context, width, height);

	m_depth_buffer_pyramid = std::make_shared<vren::depth_buffer_pyramid>(m_context, width, height);
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

	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
	{
		m_show_projected_meshlet_bounds = !m_show_projected_meshlet_bounds;
	}

	if (key == GLFW_KEY_4 && action == GLFW_PRESS)
	{
		m_show_instanced_meshlets_indices = !m_show_instanced_meshlets_indices;
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

void vren_demo::app::load_scene(char const* gltf_model_filename)
{
	VREN_INFO("[vren_demo] Loading model: {}\n", gltf_model_filename);

	vren::tinygltf_parser gltf_parser(m_context);

	vren::model parsed_model{}; // Intermediate model

	std::vector<vren::material> materials{};
	gltf_parser.load_from_file(gltf_model_filename, parsed_model, vren::k_initial_material_count, materials);

	// Upload materials
	m_material_buffer_fork.enqueue([materials](vren::material_buffer& material_buffer)
	{
		vren::material* material_buffer_ptr = material_buffer.m_buffer.get_mapped_pointer<vren::material>();

		std::memcpy(&material_buffer_ptr[material_buffer.m_material_count], materials.data(), materials.size() * sizeof(vren::material));
		material_buffer.m_material_count += materials.size();
	});

	VREN_INFO("[vren_demo] Computing AABB\n");

	parsed_model.compute_aabb();

	m_model_min = parsed_model.m_min;
	m_model_max = parsed_model.m_max;

	// Basic model
	vren::basic_model_uploader basic_model_uploader{};
	m_basic_model_draw_buffer = std::make_unique<vren::basic_model_draw_buffer>(
		basic_model_uploader.upload(m_context, parsed_model)
	);

	// Clusterized model
	vren::model_clusterizer model_clusterizer{};
	m_clusterized_model = std::make_unique<vren::clusterized_model>(
		model_clusterizer.clusterize(parsed_model)
	);

	vren::clusterized_model_uploader clusterized_model_uploader{};
	m_clusterized_model_draw_buffer = std::make_unique<vren::clusterized_model_draw_buffer>(
		clusterized_model_uploader.upload(m_context, *m_clusterized_model)
	);

	// Clusterized model debug information
	vren_demo::clusterized_model_debugger clusterized_model_debugger{};
	clusterized_model_debugger.write_debug_info_for_meshlet_geometry(*m_clusterized_model, m_debug_meshlets_draw_buffer);
	clusterized_model_debugger.write_debug_info_for_meshlet_bounds(*m_clusterized_model, m_debug_meshlet_bounds_draw_buffer);

	VREN_INFO("[vren_demo] Model completely loaded: {}\n", gltf_model_filename);
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

	m_freecam_controller.update(m_camera, dt, m_camera_speed, glm::radians(45.0f));

	if (m_point_light_speed > std::numeric_limits<float>::epsilon())
	{
		m_light_array_fork.enqueue([this, dt](vren::light_array& light_array)
		{
			vren::vk_utils::immediate_graphics_queue_submit(m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
			{
				m_point_light_bouncer.bounce(
					0,
					command_buffer,
					resource_container,
					light_array.m_point_light_position_buffer,
					m_point_light_direction_buffer,
					VREN_MAX_POINT_LIGHT_COUNT,
					glm::vec3(-1.0f),
					glm::vec3(1.0f),
					m_point_light_speed,
					dt
				);
			});
		});
	}
}

void vren_demo::app::record_commands(
	uint32_t frame_idx,
	uint32_t swapchain_image_idx,
	vren::swapchain const& swapchain,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container,
	float dt
)
{
	// We make sure that every frame is processed sequentially GPU-side
	VkMemoryBarrier memory_barrier{
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 1, &memory_barrier, 0, nullptr, 0, nullptr);

	resource_container.add_resources(
		m_depth_buffer,
		m_depth_buffer_pyramid,
		m_gbuffer
	);

	vren::light_array& light_array = m_light_arrays.at(frame_idx);
	vren::material_buffer& material_buffer = m_material_buffers.at(frame_idx);

	// Draw cartesian axes
	m_debug_draw_buffer.clear();

	m_debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(1, 0, 0), .m_color = 0xff0000 });
	m_debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 1, 0), .m_color = 0x00ff00 });
	m_debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 0, 1), .m_color = 0x0000ff });

	// Draw model AABB
	//m_debug_draw_buffer.add_cube({ .m_min = m_model_min, .m_max = m_model_max, .m_color = 0xffffff });

	// Dump profiling data
	for (uint32_t slot_idx = 0; slot_idx < ProfileSlot_Count; slot_idx++)
	{
		uint64_t time_elapsed = m_profiler.read_elapsed_time(slot_idx, frame_idx);
		if (time_elapsed != UINT64_MAX) {
			m_delta_time_by_profile_slot[slot_idx].push_value(time_elapsed);
		}
	}

	vkCmdSetCheckpointNV(command_buffer, "Frame start");
	
	glm::uvec2 screen(swapchain.m_image_width, swapchain.m_image_height);
	vren::camera_data camera_data{
		.m_position = m_camera.m_position,
		.m_view = m_camera.get_view(),
		.m_projection = m_camera.get_projection(),
		.m_z_near = m_camera.m_near_plane,
	};

	vren::vk_utils::color_buffer_t& color_buffer = m_color_buffers.at(frame_idx);

	m_light_array_fork.apply(frame_idx, light_array);
	m_material_buffer_fork.apply(frame_idx, material_buffer);

	auto render_target = vren::render_target::cover(swapchain.m_image_width, swapchain.m_image_height, color_buffer, *m_depth_buffer);

	// Render-graph begin
	vren::render_graph_builder render_graph(m_render_graph_allocator);

	// Clear color buffer
	auto clear_color_buffer = vren::clear_color_buffer(m_render_graph_allocator, color_buffer.get_image(), { 0.45f, 0.45f, 0.45f, 0.0f });
	render_graph.concat(m_profiler.profile(m_render_graph_allocator, clear_color_buffer, vren_demo::ProfileSlot_CLEAR_COLOR_BUFFER, frame_idx));

	// Clear depth buffer
	auto clear_depth_buffer = vren::clear_depth_stencil_buffer(m_render_graph_allocator, m_depth_buffer->get_image(), { .depth = 1.0f });
	render_graph.concat(m_profiler.profile(m_render_graph_allocator, clear_depth_buffer, vren_demo::ProfileSlot_CLEAR_DEPTH_BUFFER, frame_idx));

	// Render scene
	switch (m_selected_renderer_type)
	{
	case vren_demo::RendererType_NONE:
		break;
	case vren_demo::RendererType_BASIC_RENDERER:
		if (m_basic_model_draw_buffer)
		{
			auto basic_render = m_basic_renderer.render(m_render_graph_allocator, screen, m_camera, light_array, *m_basic_model_draw_buffer, *m_gbuffer, *m_depth_buffer);
			render_graph.concat(m_profiler.profile(m_render_graph_allocator, basic_render, vren_demo::ProfileSlot_BASIC_RENDERER, frame_idx));
		}
		break;
	case vren_demo::RendererType_MESH_SHADER_RENDERER:
		if (m_clusterized_model_draw_buffer)
		{
			auto mesh_shader_render = m_mesh_shader_renderer->render(m_render_graph_allocator, screen, camera_data, light_array, *m_clusterized_model_draw_buffer, *m_depth_buffer_pyramid, *m_gbuffer, *m_depth_buffer);
			render_graph.concat(m_profiler.profile(m_render_graph_allocator, mesh_shader_render, vren_demo::ProfileSlot_MESH_SHADER_RENDERER, frame_idx));

			resource_container.add_resource(m_mesh_shader_renderer);
		}
		break;
	default:
		break;
	}

	if (light_array.m_point_light_count > 0)
	{
		// Construct light_array BVH
		render_graph.concat(
			m_profiler.profile(
				m_render_graph_allocator,
				m_construct_light_array_bvh.construct(
					m_render_graph_allocator,
					light_array,
					m_point_light_bvh_buffer,
					0,
					m_point_light_index_buffer,
					0
				),
				vren_demo::ProfileSlot_CONSTRUCT_LIGHT_ARRAY_BVH
			)
		);

		// Visualize BVH
		if (m_show_light_bvh)
		{
			render_graph.concat(
				m_visualize_bvh.write(
					m_render_graph_allocator,
					m_point_light_bvh_buffer,
					vren::calc_bvh_level_count(light_array.m_point_light_count),
					m_point_light_bvh_draw_buffer
				)
			);
			m_point_light_bvh_draw_buffer.m_vertex_count = vren::calc_bvh_buffer_length(light_array.m_point_light_count) * 12 * 2;
			render_graph.concat(
				m_debug_renderer.render(
					m_render_graph_allocator,
					render_target,
					camera_data,
					m_point_light_bvh_draw_buffer
				)
			);
		}
	}

	// Cluster and shade
	m_point_light_bvh_root_index = vren::calc_bvh_root_index(light_array.m_point_light_count);

	render_graph.concat(
		m_cluster_and_shade(
			m_render_graph_allocator,
			screen,
			m_camera,
			*m_gbuffer,
			*m_depth_buffer,
			m_point_light_bvh_buffer,
			m_point_light_bvh_root_index,
			light_array.m_point_light_count,
			m_point_light_index_buffer,
			light_array,
			material_buffer,
			color_buffer
		)
	);

	// Build depth buffer pyramid
	auto build_depth_buffer_pyramid = m_depth_buffer_reductor.copy_and_reduce(m_render_graph_allocator, *m_depth_buffer, *m_depth_buffer_pyramid);
	render_graph.concat(build_depth_buffer_pyramid);

	// Debug draws
	vren::render_graph_builder debug_render_graph(m_render_graph_allocator);

	// Debug general purpose objects
	debug_render_graph.concat(m_debug_renderer.render(m_render_graph_allocator, render_target, camera_data, m_debug_draw_buffer));

	// Debug point lights
	if (light_array.m_point_light_count > 0)
	{
		/* Point lights visualization 
		vren::debug_renderer_draw_buffer& point_light_debug_draw_buffer = m_point_light_debug_draw_buffers.at(frame_idx);
		debug_render_graph.concat(
			m_fill_point_light_debug_draw_buffer(m_render_graph_allocator, light_array.m_point_light_buffer, light_array.m_point_light_count, point_light_debug_draw_buffer)
		);
		point_light_debug_draw_buffer.m_vertex_count = light_array.m_point_light_count * 6;
		debug_render_graph.concat(
			m_debug_renderer.render(m_render_graph_allocator, render_target, m_camera.to_vren(), point_light_debug_draw_buffer)
		);*/
	}

	// Debug meshlets
	if (m_show_meshlets)
	{
		debug_render_graph.concat(
			m_debug_renderer.render(
				m_render_graph_allocator,
				render_target,
				camera_data, 
				m_debug_meshlets_draw_buffer
			)
		);
	}

	// Debug meshlet bounds
	if (m_show_meshlets_bounds)
	{
		debug_render_graph.concat(
			m_debug_renderer.render(
				m_render_graph_allocator,
				render_target,
				camera_data,
				m_debug_meshlet_bounds_draw_buffer
			)
		);
	}

	// Debug projected meshlet bounds
	if (m_show_projected_meshlet_bounds)
	{
		m_debug_projected_meshlet_bounds_draw_buffer.clear();

		vren_demo::clusterized_model_debugger clusterized_model_debugger{};
		clusterized_model_debugger.write_debug_info_for_projected_sphere_bounds(*m_clusterized_model, camera_data, m_debug_projected_meshlet_bounds_draw_buffer);

		debug_render_graph.concat(m_debug_renderer.render(m_render_graph_allocator, render_target, camera_data, m_debug_projected_meshlet_bounds_draw_buffer, /* world_space */ false));
	}

	// Debug meshlet instances number
	if (m_show_instanced_meshlets_indices)
	{
		debug_render_graph.concat(m_imgui_renderer.render(m_render_graph_allocator, render_target, [&]()
		{
			for (uint32_t i = 0; i < m_clusterized_model->m_instanced_meshlets.size(); i++)
			{
				auto& instanced_meshlet = m_clusterized_model->m_instanced_meshlets.at(i);
				auto& meshlet = m_clusterized_model->m_meshlets.at(instanced_meshlet.m_meshlet_idx);
				auto& instance = m_clusterized_model->m_instances.at(instanced_meshlet.m_instance_idx);

				glm::vec3 p = instance.m_transform * glm::vec4(meshlet.m_bounding_sphere.m_center, 1.0f);

				uint32_t instanced_meshlet_color = (std::hash<uint32_t>()(i) | 0xff000000);

				auto meshlet_tag = std::to_string(i);

				auto draw_list = ImGui::GetForegroundDrawList();
				vren::imgui_utils::add_world_space_text(
					draw_list,
					camera_data,
					p,
					meshlet_tag.c_str(),
					vren::imgui_utils::to_im_color((~instanced_meshlet_color) | 0xff000000),
					vren::imgui_utils::to_im_color(instanced_meshlet_color)
				);
			}
		}));
	}

	render_graph.concat(
		m_profiler.profile(m_render_graph_allocator, debug_render_graph.get_head(), vren_demo::ProfileSlot_DEBUG_RENDERER, frame_idx)
	);

	// Debug depth buffer pyramid
	if (m_show_depth_buffer_pyramid)
	{
		debug_render_graph.concat(
			vren::blit_depth_buffer_pyramid_level_to_color_buffer(
				m_render_graph_allocator,
				*m_depth_buffer_pyramid,
				m_shown_depth_buffer_pyramid_level,
				color_buffer,
				swapchain.m_image_width, swapchain.m_image_height
			)
		);
	}

	// Show cluster keys
	if (m_show_cluster_keys)
	{
		render_graph.concat(
			m_visualize_clusters(
				m_render_graph_allocator,
				screen,
				0,
				m_cluster_and_shade.m_cluster_reference_buffer,
				m_cluster_and_shade.m_cluster_key_buffer,
				m_cluster_and_shade.m_assigned_light_buffer,
				color_buffer
			)
		);
	}

	// Show light assignment
	if (m_show_light_assignments)
	{
		render_graph.concat(
			m_visualize_clusters(
				m_render_graph_allocator,
				screen,
				1,
				m_cluster_and_shade.m_cluster_reference_buffer,
				m_cluster_and_shade.m_cluster_key_buffer,
				m_cluster_and_shade.m_assigned_light_buffer,
				color_buffer
			)
		);
	}

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
	vren::render_graph_executor executor(frame_idx, command_buffer, resource_container);
	executor.execute(m_render_graph_allocator, render_graph.get_head());

	// Take a render-graph dump if requested
	if (m_take_next_render_graph_dump)
	{
		std::ofstream output(m_render_graph_dump_file);

		vren::render_graph_dumper dumper(output);
		dumper.dump(m_render_graph_allocator, render_graph.get_head());

		m_take_next_render_graph_dump = false;
	}

	// Finally clear all created nodes and the render-graph can be allocated back
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

void vren_demo::app::on_frame(float dt)
{
	m_presenter.present([&](uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		m_frame_ended_at[frame_idx] =
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		m_frame_parallelism_pct.push_value(calc_frame_parallelism_percentage(frame_idx));

		float frame_dt = double(m_frame_ended_at[frame_idx] - m_frame_started_at[frame_idx]) / (1000.0 * 1000.0);
		m_frame_dt.push_value(frame_dt);

		m_frame_started_at[frame_idx] =
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

		m_fps_counter++;

		double now = glfwGetTime();
		if (m_last_fps_time < 0.0 || now - m_last_fps_time >= 1.0)
		{
			char win_title[256];
			sprintf(win_title, "%s (FPS: %d, last dt: %.3f ms)", "vren_demo", m_fps, frame_dt);
			glfwSetWindowTitle(m_window, win_title);

			m_fps = m_fps_counter;
			m_fps_counter = 0;
			m_last_fps_time = now;
		}

		record_commands(frame_idx, swapchain_image_idx, swapchain, command_buffer, resource_container, dt);
	});
}
