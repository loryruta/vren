
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

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

#define VREN_LOG_LEVEL VREN_LOG_LEVEL_DEBUG
#include "vren/utils/log.hpp"

#include <vren/context.hpp>
#include <vren/presenter.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/imgui_renderer.hpp>
#include <vren/utils/profiler.hpp>
#include <vren/vk_helpers/barrier.hpp>
#include <vren/utils/shapes.hpp>
#include <vren/dbg_renderer.hpp>

#include "tinygltf_loader.hpp"
#include "camera.hpp"
#include "ui.hpp"

#define VREN_DEMO_WINDOW_WIDTH  1280
#define VREN_DEMO_WINDOW_HEIGHT 720

GLFWwindow* g_window;

void create_cube(
	vren::context const& ctx,
	vren::render_object& render_obj,
	glm::vec3 position,
	glm::vec3 rotation,
	glm::vec3 scale,
	std::shared_ptr<vren::material> const& material
)
{
	vren::utils::create_cube(ctx, render_obj);

	{ // Instances
		auto transf = glm::identity<glm::mat4>();

		// Translation
		transf = glm::translate(transf, position);

		// Rotation
		transf = glm::translate(transf, glm::vec3(-0.5f));
		transf = glm::rotate(transf, rotation.x, glm::vec3(1, 0, 0));
		transf = glm::rotate(transf, rotation.y, glm::vec3(0, 1, 0));
		transf = glm::rotate(transf, rotation.z, glm::vec3(0, 0, 1));
		transf = glm::translate(transf, glm::vec3(0.5f));

		// Scaling
		transf = glm::scale(transf, scale);

		vren::instance_data inst{};
		inst.m_transform = transf;

		auto instances_buf =
			std::make_shared<vren::vk_utils::buffer>(
				vren::vk_utils::create_instances_buffer(ctx, &inst, 1)
			);
		render_obj.set_indices_buffer(instances_buf, 1);
	}

	render_obj.m_material = material;
}

void create_cube_scene(vren::context const& ctx, vren::render_list& render_list)
{
	float const surface_y = 1;
	float const surface_side = 30;
	float const r = 10;
	int const n = 50;

	{ // Surface
		auto& surface = render_list.create_render_object();

		auto mat = std::make_shared<vren::material>(ctx);
		mat->m_base_color_texture = ctx.m_toolbox->m_green_texture;
		mat->m_metallic_roughness_texture = ctx.m_toolbox->m_green_texture;
		surface.m_material = mat;

		create_cube(
			ctx,
			surface,
			glm::vec3(-surface_side / 2.0f, 0, -surface_side / 2.0f),
			glm::vec3(0),
			glm::vec3(surface_side, surface_y, surface_side),
			mat
		);
	}

	// Cubes
	for (int i = 1; i <= n; i++)
	{
		auto& cube = render_list.create_render_object();

		float cos_i = glm::cos(2 * glm::pi<float>() / (float) n * (float) i);
		float sin_i = glm::sin(2 * glm::pi<float>() / (float) n * (float) i);

		auto mat = std::make_shared<vren::material>(ctx);
		mat->m_base_color_texture = ctx.m_toolbox->m_red_texture;
		mat->m_metallic_roughness_texture = ctx.m_toolbox->m_green_texture;

		create_cube(
			ctx,
			cube,
			glm::vec3(
				cos_i * r,
				surface_y + 0.1f,
				sin_i * r
			),
			glm::vec3(0),
			glm::vec3(1),
			mat
		);
	}
}

void update_camera(float dt, vren_demo::camera& camera)
{
	const glm::vec4 k_world_up = glm::vec4(0, 1, 0, 0);

	/* Movement */
	const float k_speed = 4.0f; // m/s

	glm::vec3 dir{};
	if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) dir += -camera.get_forward();
	if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) dir += camera.get_forward();
	if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) dir += -camera.get_right();
	if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) dir += camera.get_right();
	if (glfwGetKey(g_window, GLFW_KEY_SPACE) == GLFW_PRESS) dir += camera.get_up();
	if (glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) dir += -camera.get_up();

	float inc = k_speed * dt;
	if (glfwGetKey(g_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		inc *= 100;
	}

	camera.m_position += dir * inc;

	/* Rotation */
	static std::optional<glm::dvec2> last_cur_pos;
	const glm::vec2 k_sensitivity = glm::radians(glm::vec2(90.0f)); // rad/(pixel * s)

	glm::dvec2 cur_pos{};
	glfwGetCursorPos(g_window, &cur_pos.x, &cur_pos.y);

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
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			if (glfwGetInputMode(g_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
				glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				glfwSetWindowShouldClose(g_window, true);
			}
			break;
		case GLFW_KEY_ENTER:
			if (glfwGetInputMode(g_window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
				glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

void show_point_lights(
	std::span<vren::point_light> point_lights,
	vren::dbg_renderer& dbg_renderer
)
{
	for (auto& light : point_lights)
	{
		dbg_renderer.draw_point({ .m_position = light.m_position, .m_color = glm::vec3(1, 1, 0) });
	}
}

int main(int argc, char* argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	if (argc != (1 + 1))
	{
		fprintf(stderr, "Invalid usage: <scene_filepath>");
		exit(0);
	}
	argv++;

	glfwSetErrorCallback(glfw_error_callback);

	if (glfwInit() != GLFW_TRUE)
	{
		throw std::runtime_error("Couldn't create GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	g_window = glfwCreateWindow(VREN_DEMO_WINDOW_WIDTH, VREN_DEMO_WINDOW_HEIGHT, "VRen", nullptr, nullptr);
	if (g_window == nullptr)
	{
		throw std::runtime_error("Couldn't create the window.");
	}

	glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// ---------------------------------------------------------------- Context initialization

	vren::context_info ctx_info;
	ctx_info.m_app_name = "vren_demo";
	ctx_info.m_app_version = VK_MAKE_VERSION(1, 0, 0);
	ctx_info.m_layers = {};

	uint32_t glfw_extensions_count = 0;
	char const** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
	for (int i = 0; i < glfw_extensions_count; i++) {
		ctx_info.m_extensions.push_back(glfw_extensions[i]);
	}

	ctx_info.m_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	ctx_info.m_device_extensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME); // For debug printf in shaders

	vren::context ctx(ctx_info);

	/* Renderer */
	vren::renderer renderer(ctx);
	renderer.m_clear_color = { 0.7f, 0.7f, 0.7f, 1.0f };

	/* Debug renderer */
	vren::dbg_renderer dbg_renderer(ctx);

	/* ImGui renderer */
	vren::imgui_renderer ui_renderer(ctx, g_window);

	vren::render_list render_list;
	vren::light_array light_array;

	glm::vec3 point_lights_dir[VREN_MAX_POINT_LIGHTS];
	initialize_point_lights_direction(point_lights_dir);

	/* Create surface */
	VkSurfaceKHR surf_handle;
	vren::vk_utils::check(glfwCreateWindowSurface(ctx.m_instance, g_window, nullptr, &surf_handle));
	auto surf = std::make_shared<vren::vk_surface_khr>(ctx, surf_handle);

	vren::presenter presenter(ctx, surf);

	vren_demo::ui::main_ui ui(ctx, renderer);

	vren::profiler profiler(ctx, VREN_MAX_FRAMES_IN_FLIGHT * vren_demo::profile_slot::count);

	/* */
	std::vector<vren_demo::vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<vren_demo::mesh_instance> mesh_instances;
	vren::texture_manager texture_manager;
	std::vector<vren_demo::material> materials;
	std::vector<vren_demo::mesh> meshes;

	vren_demo::tinygltf_scene loaded_scene;
	vren_demo::tinygltf_loader gltf_loader(ctx);

	printf("Loading scene: %s\n", argv[0]);

	gltf_loader.load_from_file(argv[0], vertices, indices, mesh_instances, texture_manager, materials, meshes);

	printf("Building meshlets...\n");

	const size_t max_vertices = 64;
	const size_t max_triangles = 124;
	const float cone_weight = 0.0f;

	size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<unsigned int> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<unsigned char> meshlet_triangles(max_meshlets * max_triangles * 3);

	size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), indices.data(),
												 indices.size(), reinterpret_cast<float*>(vertices.data()), vertices.size(), sizeof(vren_demo::vertex), max_vertices, max_triangles, cone_weight);

	auto vertex_buffer = vren::vk_utils::create_device_only_buffer(ctx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertices.data(), vertices.size() * sizeof(vren_demo::vertex));
	auto index_buffer = vren::vk_utils::create_device_only_buffer(ctx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_vertices.data(), meshlet_vertices.size() * sizeof(unsigned int));
	auto meshlet_buffer = vren::vk_utils::create_device_only_buffer(ctx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets.data(), meshlets.size() * sizeof(meshopt_Meshlet));

	// ---------------------------------------------------------------- Game loop

	vren_demo::camera camera{};
	glfwSetKeyCallback(g_window, on_key_press);

	int fb_width = -1, fb_height = -1;

	float last_time = -1.0;
	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		int cur_fb_width, cur_fb_height;
		glfwGetFramebufferSize(g_window, &cur_fb_width, &cur_fb_height);

		camera.m_aspect_ratio = float(cur_fb_width) / float(cur_fb_height);

		if (cur_fb_width != fb_width || cur_fb_height != fb_height)
		{
			presenter.recreate_swapchain(cur_fb_width, cur_fb_height, renderer.m_render_pass);

			fb_width = cur_fb_width;
			fb_height = cur_fb_height;
		}

		auto cur_time = (float) glfwGetTime();
		float dt = last_time >= 0 ? (cur_time - last_time) : 0;
		last_time = cur_time;

		if (glfwGetInputMode(g_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
			update_camera(dt, camera);
		}

		int win_width, win_height;
		glfwGetWindowSize(g_window, &win_width, &win_height);

		dbg_renderer.clear();

		presenter.present([&](int frame_idx, VkCommandBuffer cmd_buf, vren::resource_container& res_container, vren::render_target const& target)
        {
			/* Profiles the frame */
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

			/* Clear */
			vren::vk_utils::image_barrier(
				cmd_buf,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				target.m_color_buffer,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			vren::vk_utils::clear_color_image(cmd_buf, target.m_color_buffer, {0.45f, 0.45f, 0.45f, 0.0f});
			vren::vk_utils::image_barrier(
				cmd_buf,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				target.m_color_buffer,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			vren::vk_utils::image_barrier(
				cmd_buf,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				target.m_depth_buffer,
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
			);
			vren::vk_utils::clear_depth_image(cmd_buf, target.m_depth_buffer, { .depth = 1, .stencil = 0 });
			vren::vk_utils::image_barrier(
				cmd_buf,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				target.m_depth_buffer,
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
			);

			/* Move & show lights host-side */
			glm::vec3
				aabb_min = glm::vec3(-1.0f),
				aabb_max = glm::vec3(5.0f);

			move_and_bounce_point_lights(light_array.m_point_lights, point_lights_dir, aabb_min, aabb_max, ui.m_scene_ui.m_speed, dt);
			show_point_lights(light_array.m_point_lights, dbg_renderer);

			/* Debug renderer */
			dbg_renderer.draw_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(1, 0, 0), .m_color = glm::vec4(1, 0, 0, 1) });
			dbg_renderer.draw_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 1, 0), .m_color = glm::vec4(0, 1, 0, 1) });
			dbg_renderer.draw_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 0, 1), .m_color = glm::vec4(0, 0, 1, 1) });

			dbg_renderer.draw_point({ .m_position = aabb_min, .m_color = glm::vec4(0) });
			dbg_renderer.draw_point({ .m_position = aabb_max, .m_color = glm::vec4(1) });
			dbg_renderer.draw_line({ .m_from = aabb_min, .m_to = aabb_max, .m_color = glm::vec4(1, 1, 1, 1) });

			dbg_renderer.render(frame_idx, cmd_buf, res_container, target, {
				.m_camera_view = camera.get_view(),
				.m_camera_projection = camera.get_projection()
			});

			/* Renders the scene */
			profiler.profile(frame_idx, cmd_buf, res_container, prof_slot + vren_demo::profile_slot::MainPass, [&]()
			{
				auto cam_data = vren::camera{
					.m_position = camera.m_position,
					.m_view = camera.get_view(),
					.m_projection = camera.get_projection()
				};

				renderer.render(
					frame_idx,
					cmd_buf,
					res_container,
					target,
					vertex_buffer.m_buffer.m_handle,
					index_buffer.m_buffer.m_handle,
					meshlet_buffer.m_buffer.m_handle,
					meshlets.size(),
					light_array,
					cam_data
				);
			});

			/* Renders the UI */
			profiler.profile(frame_idx, cmd_buf, res_container, prof_slot + vren_demo::profile_slot::UiPass, [&]()
			{
				ui_renderer.render(frame_idx, cmd_buf, res_container, target, [&]()
				{
					ui.m_fps_ui.notify_frame_profiling_data(prof_info);

					ui.show(render_list, light_array);
				});
			});
		});
	}

	vkDeviceWaitIdle(ctx.m_device);

	//glfwDestroyWindow(g_window); // TODO Terminate only AFTER Vulkan instance destruction!
	//glfwTerminate();
}
