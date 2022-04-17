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

#include <vren/mesh.hpp>
#include <vren/context.hpp>
#include <vren/presenter.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/imgui_renderer.hpp>
#include <vren/utils/profiler.hpp>
#include <vren/vk_helpers/barrier.hpp>
#include <vren/debug_renderer.hpp>

#include "tinygltf_loader.hpp"
#include "camera.hpp"
#include "ui.hpp"

void update_camera(GLFWwindow* window, float dt, vren_demo::camera& camera)
{
	const glm::vec4 k_world_up = glm::vec4(0, 1, 0, 0);

	/* Movement */
	const float k_speed = 4.0f; // m/s

	glm::vec3 dir{};
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir += -camera.get_forward();
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir += camera.get_forward();
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir += -camera.get_right();
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir += camera.get_right();
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) dir += camera.get_up();
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) dir += -camera.get_up();

	float inc = k_speed * dt;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		inc *= 100;
	}

	camera.m_position += dir * inc;

	/* Rotation */
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

void show_point_lights(
	std::span<vren::point_light> point_lights,
	vren::debug_renderer& dbg_renderer
)
{
	for (auto& light : point_lights)
	{
		dbg_renderer.draw_point({ .m_position = light.m_position, .m_color = glm::vec3(1, 1, 0) });
	}
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
	auto renderer = std::make_unique<vren::renderer>(context);
	auto debug_renderer = std::make_unique<vren::debug_renderer>(context);
	auto imgui_renderer = std::make_unique<vren::imgui_renderer>(context, window);

	/* Presenter init */
	VkSurfaceKHR surface_handle;
	VREN_CHECK(glfwCreateWindowSurface(context.m_instance, window, nullptr, &surface_handle), &context);
	auto surface = std::make_shared<vren::vk_surface_khr>(context, surface_handle);

	/* Framebuffers */
	std::vector<vren::vk_framebuffer> renderer_framebuffers;
	std::vector<vren::vk_framebuffer> debug_renderer_framebuffers;
	std::vector<vren::vk_framebuffer> imgui_renderer_framebuffers;

	vren::presenter presenter(context, surface, [&](vren::swapchain const& swapchain)
	{
		size_t image_count = swapchain.m_images.size();

		renderer_framebuffers.clear();
		debug_renderer_framebuffers.clear();
		imgui_renderer_framebuffers.clear();

		for (uint32_t image_idx = 0; image_idx < image_count; image_idx++)
		{
			VkImageView attachments[]{
				swapchain.m_color_buffers.at(image_idx).m_image_view.m_handle,
				swapchain.m_depth_buffer.m_image_view.m_handle
			};
			renderer_framebuffers.push_back(
				vren::vk_utils::create_framebuffer(context, renderer->m_render_pass.m_handle, attachments, swapchain.m_image_width, swapchain.m_image_height)
			);
			debug_renderer_framebuffers.push_back(
				vren::vk_utils::create_framebuffer(context, debug_renderer->m_render_pass.m_handle, attachments, swapchain.m_image_width, swapchain.m_image_height)
			);
			imgui_renderer_framebuffers.push_back(
				vren::vk_utils::create_framebuffer(context, imgui_renderer->m_render_pass.m_handle, attachments, swapchain.m_image_width, swapchain.m_image_height)
			);
		}
	});

	vren_demo::ui::main_ui ui(context, *renderer);

	/* Profiler */
	vren::profiler profiler(context, VREN_MAX_FRAME_IN_FLIGHT_COUNT * vren_demo::profile_slot::count);

	//glm::vec3 point_lights_dir[VREN_MAX_POINT_LIGHTS];
	//initialize_point_lights_direction(point_lights_dir);

	/* */
	std::vector<vren::vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<vren::mesh> meshes;
	std::vector<vren::mesh_instance> mesh_instances;

	vren_demo::tinygltf_scene loaded_scene;
	vren_demo::tinygltf_loader gltf_loader(context);

	printf("Loading scene: %s\n", argv[0]);

	gltf_loader.load_from_file(argv[0], vertices, indices, meshes, mesh_instances);

	/*
	printf("Building meshlets...\n");

	const size_t max_vertices = 64;
	const size_t max_triangles = 124;
	const float cone_weight = 0.0f;

	size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), max_vertices, max_triangles);
	std::vector<meshopt_Meshlet> meshlets(max_meshlets);
	std::vector<uint32_t> meshlet_vertices(max_meshlets * max_vertices);
	std::vector<uint8_t> meshlet_triangles(max_meshlets * max_triangles * 3);

	size_t meshlet_count = meshopt_buildMeshlets(
		meshlets.data(),
		meshlet_vertices.data(),
		meshlet_triangles.data(),
		indices.data(),
		indices.size(),
		reinterpret_cast<float*>(vertices.data()),
		vertices.size(),
		sizeof(vren::vertex),
		max_vertices,
		max_triangles,
		cone_weight
	);

	meshopt_Meshlet const& last = meshlets[meshlet_count - 1];

	meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
	meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
	meshlets.resize(meshlet_count);

	vren::draw_buffer draw_buffer{
		.m_vertex_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertices.data(), vertices.size() * sizeof(vren::vertex)),
		.m_meshlet_vertex_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_vertices.data(), meshlet_vertices.size() * sizeof(uint32_t)),
		.m_meshlet_triangle_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlet_triangles.data(), meshlet_triangles.size() * sizeof(uint8_t)),
		.m_meshlet_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshlets.data(), meshlets.size() * sizeof(meshopt_Meshlet)),
		.m_meshlet_count = meshlet_count
	};
	*/

	/* */
	std::vector<vren::mesh_buffer> mesh_buffers;
	for (auto const& mesh : meshes)
	{
		vren::mesh_buffer mesh_buffer{
			.m_vertex_buffer   = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vertices.data()[mesh.m_vertex_offset], mesh.m_vertex_count * sizeof(vren::vertex)),
			.m_vertex_count    = mesh.m_vertex_count,
			.m_index_buffer    = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &indices.data()[mesh.m_index_offset], mesh.m_index_count * sizeof(uint32_t)),
			.m_index_count     = mesh.m_index_count,
			.m_instance_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &mesh_instances.data()[mesh.m_instance_offset], mesh.m_instance_count * sizeof(vren::mesh_instance)),
			.m_instance_count  = mesh.m_instance_count,
			.m_material_index_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &mesh.m_material_idx, sizeof(uint32_t))
		};
		mesh_buffers.push_back(std::move(mesh_buffer));
	}

	// ---------------------------------------------------------------- Game loop

	printf("Game loop! (ノ ゜Д゜)ノ ︵ ┻━┻\n");

	vren_demo::camera camera{};
	glfwSetKeyCallback(window, on_key_press);

	int fb_width = -1, fb_height = -1;

	float last_time = -1.0;
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		VmaBudget budget;
		vmaGetBudget(context.m_vma_allocator, &budget);

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
			update_camera(window, dt, camera);
		}

		int win_width, win_height;
		glfwGetWindowSize(window, &win_width, &win_height);

		debug_renderer->clear();

		presenter.present([&](uint32_t frame_idx, uint32_t swapchain_image_idx, vren::swapchain const& swapchain, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
        {
			vkCmdSetCheckpointNV(command_buffer, "Frame start");

			context.m_toolbox->m_material_manager.sync_buffer(frame_idx);

			vren::render_target render_target{
				.m_render_area = {
					.offset = {0, 0},
					.extent = {swapchain.m_image_width, swapchain.m_image_height}
				},
				.m_viewport = {
					.x = 0,
					.y = (float) swapchain.m_image_height,
					.width = (float) swapchain.m_image_width,
					.height = -((float) swapchain.m_image_height),
					.minDepth = 0.0f,
					.maxDepth = 1.0f
				},
				.m_scissor {
					.offset = {0,0},
					.extent = {0,0}
				}
			};

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

			VkImage color_buffer = swapchain.m_color_buffers.at(swapchain_image_idx).m_image;
			vren::vk_utils::image_barrier(
				command_buffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				color_buffer,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			vren::vk_utils::clear_color_image(command_buffer, color_buffer, {0.45f, 0.45f, 0.45f, 0.0f});
			vren::vk_utils::image_barrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				color_buffer,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
			vkCmdSetCheckpointNV(command_buffer, "Color buffer cleared");

			VkImage depth_buffer = swapchain.m_depth_buffer.m_image.m_image.m_handle;
			vren::vk_utils::image_barrier(
				command_buffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				NULL, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				depth_buffer,
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
			);
			vren::vk_utils::clear_depth_image(command_buffer, depth_buffer, { .depth = 1, .stencil = 0 });
			vren::vk_utils::image_barrier(
				command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				depth_buffer,
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
			);
			vkCmdSetCheckpointNV(command_buffer, "Depth buffer cleared");

			glm::vec3
				aabb_min = glm::vec3(-1.0f),
				aabb_max = glm::vec3(5.0f);

			//move_and_bounce_point_lights(light_array.m_point_lights, point_lights_dir, aabb_min, aabb_max, ui.m_scene_ui.m_speed, dt);
			//show_point_lights(light_array.m_point_lights, dbg_renderer);

			debug_renderer->draw_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(1, 0, 0), .m_color = glm::vec4(1, 0, 0, 1) });
			debug_renderer->draw_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 1, 0), .m_color = glm::vec4(0, 1, 0, 1) });
			debug_renderer->draw_line({ .m_from = glm::vec3(0), .m_to = glm::vec3(0, 0, 1), .m_color = glm::vec4(0, 0, 1, 1) });

			debug_renderer->draw_point({ .m_position = aabb_min, .m_color = glm::vec4(0) });
			debug_renderer->draw_point({ .m_position = aabb_max, .m_color = glm::vec4(1) });
			debug_renderer->draw_line({ .m_from = aabb_min, .m_to = aabb_max, .m_color = glm::vec4(1, 1, 1, 1) });

			render_target.m_framebuffer = debug_renderer_framebuffers.at(swapchain_image_idx).m_handle;
			debug_renderer->render(frame_idx, command_buffer, resource_container, render_target, {
				.m_camera_view = camera.get_view(),
				.m_camera_projection = camera.get_projection()
			});
			vkCmdSetCheckpointNV(command_buffer, "Debug renderer drawn");

			// Render scene
			profiler.profile(frame_idx, command_buffer, resource_container, prof_slot + vren_demo::profile_slot::MainPass, [&]()
			{
				auto cam_data = vren::camera{
					.m_position = camera.m_position,
					.m_view = camera.get_view(),
					.m_projection = camera.get_projection()
				};
				render_target.m_framebuffer = renderer_framebuffers.at(swapchain_image_idx).m_handle;
				renderer->render(frame_idx, command_buffer, resource_container, render_target, cam_data, mesh_buffers);
				vkCmdSetCheckpointNV(command_buffer, "Scene drawn");
			});

			profiler.profile(frame_idx, command_buffer, resource_container, prof_slot + vren_demo::profile_slot::UiPass, [&]()
			{
				render_target.m_framebuffer = imgui_renderer_framebuffers.at(swapchain_image_idx).m_handle;
				imgui_renderer->render(frame_idx, command_buffer, resource_container, render_target, [&]()
				{
					ui.m_fps_ui.notify_frame_profiling_data(prof_info);

					ui.show(renderer->m_light_array);
				});
				vkCmdSetCheckpointNV(command_buffer, "ImGui drawn");
			});

			vkCmdSetCheckpointNV(command_buffer, "Frame end");
		});
	}

	vkDeviceWaitIdle(context.m_device);

	//glfwDestroyWindow(g_window); // TODO Terminate only AFTER Vulkan instance destruction!
	//glfwTerminate();

	return 0;
}
