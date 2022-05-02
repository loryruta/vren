#include <memory>
#include <iostream>
#include <optional>
#include <numeric>

#include <volk.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/random.hpp>
#include <meshoptimizer.h>

#include <vren/utils/log.hpp>

#include <vren/gpu_repr.hpp>
#include <vren/context.hpp>
#include <vren/presenter.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/imgui_renderer.hpp>
#include <vren/utils/profiler.hpp>
#include <vren/vk_helpers/barrier.hpp>
#include <vren/render_graph.hpp>
#include <vren/debug_renderer.hpp>
#include <vren/depth_buffer_pyramid.hpp>

#include "scene/tinygltf_parser.hpp"
#include "scene/scene_gpu_uploader.hpp"
#include "camera.hpp"
#include "ui.hpp"

class renderer_type
{
public:
	enum enum_t
	{
		NONE,
		BASIC_RENDERER,
		MESH_SHADER_RENDERER,

		Count
	};
};

auto g_renderer_type = renderer_type::BASIC_RENDERER;
bool g_show_meshlets = false;
bool g_show_meshlet_bounds = false;

void update_camera(GLFWwindow* window, float dt, vren_demo::camera& camera, float camera_speed)
{
	const glm::vec4 k_world_up = glm::vec4(0, 1, 0, 0);

	// Movement
	glm::vec3 direction{};
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) direction += -camera.get_forward();
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) direction += camera.get_forward();
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) direction += -camera.get_right();
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) direction += camera.get_right();
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) direction += camera.get_up();
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) direction += -camera.get_up();

	float step = camera_speed * dt;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		step *= 100;
	}

	camera.m_position += direction * step;

	// Rotation
	static std::optional<glm::dvec2> last_cur_pos;
	const glm::vec2 k_sensitivity = glm::radians(glm::vec2(90.0f)); // rad/(pixel * s)

	glm::dvec2 cur_pos{};
	glfwGetCursorPos(window, &cur_pos.x, &cur_pos.y);

	if (last_cur_pos.has_value())
	{
		glm::vec2 d_cur_pos = glm::vec2(cur_pos - last_cur_pos.value()) * k_sensitivity * dt;
		camera.m_yaw += d_cur_pos.x;
		camera.m_pitch = glm::clamp(camera.m_pitch + (float) d_cur_pos.y, -glm::pi<float>() / 4.0f, glm::pi<float>() / 4.0f);
	}

	last_cur_pos = cur_pos;
}

void on_key_press(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		g_show_meshlets = !g_show_meshlets;
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		g_show_meshlet_bounds = !g_show_meshlet_bounds;
	}

	if (key == GLFW_KEY_F2 && action == GLFW_PRESS) {
		g_renderer_type = static_cast<renderer_type::enum_t>((g_renderer_type + 1) % renderer_type::enum_t::Count);
	}

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				glfwSetWindowShouldClose(window, true);
			}
			break;
		case GLFW_KEY_ENTER:
			if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			break;
		}
	}
}

void glfw_error_callback(int error_code, const char* description)
{
	std::cerr << "[GLFW] (" << error_code << ") " << description << std::endl;
}

void initialize_point_lights_direction(std::span<glm::vec3> point_lights_dir)
{
	for (size_t i = 0; i < point_lights_dir.size(); i++) {
		point_lights_dir[i] = glm::ballRand(1.0f);
	}
}

void move_and_bounce_point_lights(
	std::span<vren::point_light> point_lights,
	std::span<glm::vec3> point_lights_dir,
	glm::vec3 const& sm,
	glm::vec3 const& sM,
	float speed,
	float dt
)
{
	for (size_t i = 0; i < point_lights.size(); i++)
	{
		auto& light = point_lights[i];

		glm::vec3 p = light.m_position;
		p = glm::min(glm::max(p, sm), sM);

		glm::vec3 d = point_lights_dir[i];

		float rem_t = speed * dt;

		for (uint32_t j = 0; j < 256 && rem_t > 0; j++)
		{
			float tx1 = (sm.x - p.x) / d.x; tx1 = tx1 <= 0 ? std::numeric_limits<float>::infinity() : tx1;
			float tx2 = (sM.x - p.x) / d.x; tx2 = tx2 <= 0 ? std::numeric_limits<float>::infinity() : tx2;
			float ty1 = (sm.y - p.y) / d.y; ty1 = ty1 <= 0 ? std::numeric_limits<float>::infinity() : ty1;
			float ty2 = (sM.y - p.y) / d.y; ty2 = ty2 <= 0 ? std::numeric_limits<float>::infinity() : ty2;
			float tz1 = (sm.z - p.z) / d.z; tz1 = tz1 <= 0 ? std::numeric_limits<float>::infinity() : tz1;
			float tz2 = (sM.z - p.z) / d.z; tz2 = tz2 <= 0 ? std::numeric_limits<float>::infinity() : tz2;

			float min_t = glm::min(tx1, glm::min(tx2, glm::min(ty1, glm::min(ty2, glm::min(tz1, tz2)))));

			float step = glm::min(min_t - std::numeric_limits<float>::epsilon(), rem_t);
			p += step * d;

			if (min_t < rem_t)
			{
				d *= glm::vec3(
					(tx1 == min_t || tx2 == min_t ? -1.0 : 1.0),
					(ty1 == min_t || ty2 == min_t ? -1.0 : 1.0),
					(tz1 == min_t || tz2 == min_t ? -1.0 : 1.0)
				);
			}

			rem_t -= step;
		}

		light.m_position = p;
		point_lights_dir[i] = d;
	}
}

vren::mesh_shader_renderer_draw_buffer upload_scene_for_mesh_shader_renderer(
	vren::context const& context,
	vren_demo::intermediate_scene const& parsed_scene,
	vren::debug_renderer_draw_buffer& meshlet_debug_draw_buffer,
	vren::debug_renderer_draw_buffer& meshlet_bounds_debug_draw_buffer
)
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

int main(int argc, char* argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout/stderr buffering
	setvbuf(stderr, NULL, _IONBF, 0);

	if (argc != (1 + 1))
	{
		VREN_ERROR("Invalid usage: <scene_filepath>\n");
		exit(1);
	}
	argv++;

	/* GLFW window init */
	glfwSetErrorCallback(glfw_error_callback);

	if (glfwInit() != GLFW_TRUE)
	{
		VREN_ERROR("Failed to initialize GLFW\n");
		exit(1);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(1024, 720, "vren_demo", nullptr, nullptr);
	if (window == nullptr)
	{
		VREN_ERROR("Failed to create window\n");
		exit(1);
	}

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	/* Context init */
	vren::context_info context_info{
		.m_app_name = "vren_demo",
		.m_app_version = VK_MAKE_VERSION(1, 0, 0),
		.m_layers = {},
		.m_extensions = {},
		.m_device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		},
	};

	uint32_t glfw_extensions_count = 0;
	char const** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
	for (int i = 0; i < glfw_extensions_count; i++) {
		context_info.m_extensions.push_back(glfw_extensions[i]);
	}

	vren::context context(context_info);

	/* Renderer init */
	auto basic_renderer = std::make_unique<vren::basic_renderer>(context);
	auto mesh_shader_renderer = std::make_unique<vren::mesh_shader_renderer>(context);
	auto debug_renderer = std::make_unique<vren::debug_renderer>(context);
	auto imgui_renderer = std::make_unique<vren::imgui_renderer>(context, window);

	vren::debug_renderer_draw_buffer debug_draw_buffer(context); // General purpose debug draw buffer

	auto light_array = std::make_unique<vren::light_array>(context);

	/* Presenter init */
	VkSurfaceKHR surface_handle;
	VREN_CHECK(glfwCreateWindowSurface(context.m_instance, window, nullptr, &surface_handle), &context);
	auto surface = std::make_shared<vren::vk_surface_khr>(context, surface_handle);

	// Color buffers
	std::vector<vren::vk_utils::color_buffer_t> color_buffers;

	// Depth buffer
	std::unique_ptr<vren::vk_utils::depth_buffer_t> depth_buffer;

	// Depth buffer pyramid
	std::unique_ptr<vren::depth_buffer_pyramid> depth_buffer_pyramid;

	// Presenter
	vren::presenter presenter(context, surface, [&](vren::swapchain const& swapchain) {
		// Re-create color buffers
		color_buffers.clear();
		for (uint32_t i = 0; i < swapchain.m_images.size(); i++) {
			color_buffers.push_back(
				vren::vk_utils::create_color_buffer(context, swapchain.m_image_width, swapchain.m_image_height, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			);
		}

		// Re-create depth buffer
		uint32_t padded_width = vren::round_to_nearest_multiple_of_power_of_2(swapchain.m_image_width, 32); // The depth buffer size must be padded to a multiple of 32 for efficiency reasons
		uint32_t padded_height = vren::round_to_nearest_multiple_of_power_of_2(swapchain.m_image_height, 32);

		depth_buffer = std::make_unique<vren::vk_utils::depth_buffer_t>(
			vren::vk_utils::create_depth_buffer(context, padded_width, padded_height, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		);

		// Re-create depth buffer pyramid
		depth_buffer_pyramid = std::make_unique<vren::depth_buffer_pyramid>(context, padded_width, padded_height);
	});

	// Depth buffer pyramid reductor
	vren::depth_buffer_reductor depth_buffer_reductor(context); // Used to build up the depth buffer pyramid

	// UI
	vren_demo::ui::main_ui ui(context, *basic_renderer);

	/* Profiler */
	vren::profiler profiler(context, VREN_MAX_FRAME_IN_FLIGHT_COUNT * vren_demo::profile_slot::count);

	//glm::vec3 point_lights_dir[VREN_MAX_POINT_LIGHTS];
	//initialize_point_lights_direction(point_lights_dir);

	/* */
	vren_demo::intermediate_scene parsed_scene;

	printf("Parsing scene: %s\n", argv[0]);

	vren_demo::tinygltf_parser gltf_parser(context);
	gltf_parser.load_from_file(argv[0], parsed_scene);

	printf("Uploading scene for basic renderer...\n");
	auto basic_renderer_draw_buffer = vren_demo::upload_scene_for_basic_renderer(context, parsed_scene);

	printf("Uploading scene for mesh shader renderer...\n");

	vren::debug_renderer_draw_buffer meshlet_debug_draw_buffer(context);
	vren::debug_renderer_draw_buffer meshlet_bounds_debug_draw_buffer(context);

	auto mesh_shader_renderer_draw_buffer = upload_scene_for_mesh_shader_renderer(context, parsed_scene, meshlet_debug_draw_buffer, meshlet_bounds_debug_draw_buffer);

	// Cartesian axes
	debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(1, 0, 0), .m_color = 0xff0000 });
	debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 1, 0), .m_color = 0x00ff00 });
	debug_draw_buffer.add_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 0, 1), .m_color = 0x0000ff });

	// ---------------------------------------------------------------- Game loop

	printf("Game loop! (ノ ゜Д゜)ノ ︵ ┻━┻\n");

	vren_demo::camera camera{};
	glfwSetKeyCallback(window, on_key_press);

	int fb_width = -1, fb_height = -1;

	float last_time = -1.0;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		int cur_fb_width, cur_fb_height;
		glfwGetFramebufferSize(window, &cur_fb_width, &cur_fb_height);

		camera.m_aspect_ratio = float(cur_fb_width) / float(cur_fb_height);

		if (cur_fb_width != fb_width || cur_fb_height != fb_height)
		{
			presenter.recreate_swapchain(cur_fb_width, cur_fb_height);

			fb_width = cur_fb_width;
			fb_height = cur_fb_height;
		}

		auto cur_time = (float) glfwGetTime();
		float dt = last_time >= 0 ? (cur_time - last_time) : 0;
		last_time = cur_time;

		if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
		{
			update_camera(window, dt, camera, 0.1f);
		}

		int win_width, win_height;
		glfwGetWindowSize(window, &win_width, &win_height);

		vren::camera camera_data{ .m_position = camera.m_position, .m_view = camera.get_view(), .m_projection = camera.get_projection() };

		presenter.present([&](uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
			vkCmdSetCheckpointNV(command_buffer, "Frame start");

			auto& color_buffer = color_buffers.at(frame_idx);

			vren::render_target render_target = vren::render_target::cover(swapchain.m_image_width, swapchain.m_image_height, color_buffer, *depth_buffer);

			// Material uploading
			context.m_toolbox->m_material_manager.sync_buffer(frame_idx, command_buffer);

			// Light array uploading
			light_array->sync_buffers(frame_idx);

			// Profiling
			int prof_slot = frame_idx * vren_demo::profile_slot::count;

			vren_demo::profile_info prof_info{};

			prof_info.m_frame_idx = frame_idx;
			prof_info.m_frame_in_flight_count = presenter.m_swapchain->m_frame_data.size();

			prof_info.m_main_pass_profiled =
				profiler.get_timestamps(prof_slot + vren_demo::profile_slot::MainPass, &prof_info.m_main_pass_start_t, &prof_info.m_main_pass_end_t);

			prof_info.m_ui_pass_profiled =
				profiler.get_timestamps(prof_slot + vren_demo::profile_slot::UiPass, &prof_info.m_ui_pass_start_t, &prof_info.m_ui_pass_end_t);

			prof_info.m_frame_profiled = prof_info.m_main_pass_profiled && prof_info.m_ui_pass_profiled;
			prof_info.m_frame_start_t = prof_info.m_main_pass_start_t;
			prof_info.m_frame_end_t = prof_info.m_ui_pass_end_t;

			// Render-graph begin
			vren::render_graph_node* render_graph_head;
			vren::render_graph_node* node;

			// Clear color buffer
			node = vren::clear_color_buffer(color_buffer.get_image(), { 0.45f, 0.45f, 0.45f, 0.0f });
			render_graph_head = node;

			// Clear depth buffer
			node = node->chain(
				vren::clear_depth_stencil_buffer(depth_buffer->get_image(), { .depth = 1.0f, .stencil = 0 })
			);

			// Render scene
			if (g_renderer_type == renderer_type::BASIC_RENDERER)
			{
				node = node->chain(
					basic_renderer->render(
						render_target,
						{ .m_position = camera.m_position, .m_view = camera.get_view(), .m_projection = camera.get_projection() },
						*light_array,
						basic_renderer_draw_buffer
					)
				);
			}
			else if (g_renderer_type == renderer_type::MESH_SHADER_RENDERER)
			{
				node = node->chain(
					mesh_shader_renderer->render(
						render_target,
						{ .m_position = camera.m_position, .m_view = camera.get_view(), .m_projection = camera.get_projection() },
						*light_array,
						mesh_shader_renderer_draw_buffer
					)
				);
			}

			// Build depth buffer pyramid
			node = node->chain(
				depth_buffer_reductor.copy_and_reduce(*depth_buffer, *depth_buffer_pyramid)
			);
			while (node->get_following_nodes().size() > 0) {
				node = node->get_following_nodes()[0];
			}

			// Debug general purpose objects
			node = node->chain(
				debug_renderer->render(render_target, camera_data, debug_draw_buffer)
			);

			// Debug meshlets
			if (g_show_meshlets)
			{
				node = node->chain(
					debug_renderer->render(render_target, camera_data, meshlet_debug_draw_buffer)
				);
			}

			// Debug meshlet bounds
			if (g_show_meshlet_bounds)
			{
				node = node->chain(
					debug_renderer->render(render_target, camera_data, meshlet_bounds_debug_draw_buffer)
				);
			}

			// Debug depth buffer pyramid
			if (ui.m_depth_buffer_pyramid_ui.m_show)
			{
				node = node->chain(
					vren::blit_depth_buffer_pyramid_level_to_color_buffer(*depth_buffer_pyramid, ui.m_depth_buffer_pyramid_ui.m_selected_level, color_buffer, swapchain.m_image_width, swapchain.m_image_height)
				);
			}

			// Render UI
			node = node->chain(
				imgui_renderer->render(render_target, [&]() {
					ui.m_fps_ui.notify_frame_profiling_data(prof_info);
					ui.show(*depth_buffer_pyramid, *light_array);
				})
			);

			// Blit to swapchain image
			node = node->chain(
				vren::blit_color_buffer_to_swapchain_image(
					color_buffer,
					swapchain.m_image_width, swapchain.m_image_height,
					swapchain.m_images.at(swapchain_image_idx),
					swapchain.m_image_width, swapchain.m_image_height
				)
			);

			// Ensure swapchain image is in present layout
			node = node->chain(
				vren::transit_swapchain_image_to_present_layout(swapchain.m_images.at(swapchain_image_idx))
			);

			// Render-graph end
			(void) node;

			// Execute render-graph
			vren::render_graph_handler render_graph({ render_graph_head });

			vren::render_graph_executor render_graph_executor;
			render_graph_executor.execute(render_graph.get_handle(), frame_idx, command_buffer, resource_container);
		});
	}

	vkDeviceWaitIdle(context.m_device);

	//glfwDestroyWindow(g_window); // TODO Terminate only AFTER Vulkan instance destruction!
	//glfwTerminate();

	return 0;
}
