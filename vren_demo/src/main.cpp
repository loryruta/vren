#include <memory>
#include <iostream>
#include <optional>
#include <numeric>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "context.hpp"
#include "utils/vk_toolbox.hpp"
#include "presenter.hpp"
#include "utils/buffer.hpp"
#include "camera.hpp"
#include "tinygltf_loader.hpp"
#include "imgui_renderer.hpp"
#include "ui.hpp"
#include "utils/profiler.hpp"

#include "profile.hpp"

#define VREN_DEMO_WINDOW_WIDTH  1280
#define VREN_DEMO_WINDOW_HEIGHT 720

GLFWwindow* g_window;

void create_cube(
	vren::vk_utils::toolbox const& tb,
	vren::render_object& render_obj,
	glm::vec3 position,
	glm::vec3 rotation,
	glm::vec3 scale,
	std::shared_ptr<vren::material> const& material
)
{
	std::vector<vren::vertex> vertices = {
		// Bottom face
		vren::vertex{ .m_position = { 0, 0, 1 }, .m_normal = { 0, -1, 0 } },
		vren::vertex{ .m_position = { 0, 0, 0 }, .m_normal = { 0, -1, 0 } },
		vren::vertex{ .m_position = { 1, 0, 0 }, .m_normal = { 0, -1, 0 } },
		vren::vertex{ .m_position = { 1, 0, 0 }, .m_normal = { 0, -1, 0 } },
		vren::vertex{ .m_position = { 1, 0, 1 }, .m_normal = { 0, -1, 0 } },
		vren::vertex{ .m_position = { 0, 0, 1 }, .m_normal = { 0, -1, 0 } },

		// Top face
		vren::vertex{ .m_position = { 0, 1, 1 }, .m_normal = { 0, 1, 0 } },
		vren::vertex{ .m_position = { 0, 1, 0 }, .m_normal = { 0, 1, 0 } },
		vren::vertex{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 1, 0 } },
		vren::vertex{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 1, 0 } },
		vren::vertex{ .m_position = { 1, 1, 1 }, .m_normal = { 0, 1, 0 } },
		vren::vertex{ .m_position = { 0, 1, 1 }, .m_normal = { 0, 1, 0 } },

		// Left face
		vren::vertex{ .m_position = { 0, 1, 0 }, .m_normal = { -1, 0, 0 } },
		vren::vertex{ .m_position = { 0, 0, 0 }, .m_normal = { -1, 0, 0 } },
		vren::vertex{ .m_position = { 0, 0, 1 }, .m_normal = { -1, 0, 0 } },
		vren::vertex{ .m_position = { 0, 0, 1 }, .m_normal = { -1, 0, 0 } },
		vren::vertex{ .m_position = { 0, 1, 1 }, .m_normal = { -1, 0, 0 } },
		vren::vertex{ .m_position = { 0, 1, 0 }, .m_normal = { -1, 0, 0 } },

		// Right face
		vren::vertex{ .m_position = { 1, 1, 0 }, .m_normal = { 1, 0, 0 } },
		vren::vertex{ .m_position = { 1, 0, 0 }, .m_normal = { 1, 0, 0 } },
		vren::vertex{ .m_position = { 1, 0, 1 }, .m_normal = { 1, 0, 0 } },
		vren::vertex{ .m_position = { 1, 0, 1 }, .m_normal = { 1, 0, 0 } },
		vren::vertex{ .m_position = { 1, 1, 1 }, .m_normal = { 1, 0, 0 } },
		vren::vertex{ .m_position = { 1, 1, 0 }, .m_normal = { 1, 0, 0 } },

		// Back face
		vren::vertex{ .m_position = { 0, 0, 0 }, .m_normal = { 0, 0, -1 } },
		vren::vertex{ .m_position = { 0, 1, 0 }, .m_normal = { 0, 0, -1 } },
		vren::vertex{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 0, -1 } },
		vren::vertex{ .m_position = { 1, 1, 0 }, .m_normal = { 0, 0, -1 } },
		vren::vertex{ .m_position = { 1, 0, 0 }, .m_normal = { 0, 0, -1 } },
		vren::vertex{ .m_position = { 0, 0, 0 }, .m_normal = { 0, 0, -1 } },

		// Front face
		vren::vertex{ .m_position = { 0, 0, 1 }, .m_normal = { 0, 0, 1 } },
		vren::vertex{ .m_position = { 0, 1, 1 }, .m_normal = { 0, 0, 1 } },
		vren::vertex{ .m_position = { 1, 1, 1 }, .m_normal = { 0, 0, 1 } },
		vren::vertex{ .m_position = { 1, 1, 1 }, .m_normal = { 0, 0, 1 } },
		vren::vertex{ .m_position = { 1, 0, 1 }, .m_normal = { 0, 0, 1 } },
		vren::vertex{ .m_position = { 0, 0, 1 }, .m_normal = { 0, 0, 1 } },
	};

	auto vertices_buf =
		std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::create_vertex_buffer(tb, vertices.data(), vertices.size())
		);
	render_obj.set_vertices_buffer(vertices_buf, vertices.size());

	// Indices
	std::vector<uint32_t> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	auto indices_buf =
		std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::create_indices_buffer(tb, indices.data(), indices.size())
		);
	render_obj.set_indices_buffer(indices_buf, indices.size());

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
				vren::vk_utils::create_instances_buffer(tb, &inst, 1)
			);
		render_obj.set_indices_buffer(instances_buf, 1);
	}


	render_obj.m_material = material;
}

void create_cube_scene(
	vren::vk_utils::toolbox const& tb,
	vren::render_list& render_list
)
{
	float const surface_y = 1;
	float const surface_side = 30;
	float const r = 10;
	int const n = 50;

	{ // Surface
		auto& surface = render_list.create_render_object();

		auto mat = std::make_shared<vren::material>(tb);
		mat->m_base_color_texture = tb.m_green_texture;
		mat->m_metallic_roughness_texture = tb.m_green_texture;
		surface.m_material = mat;

		create_cube(
			tb,
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

		auto mat = std::make_shared<vren::material>(tb);
		mat->m_base_color_texture = tb.m_red_texture;
		mat->m_metallic_roughness_texture = tb.m_green_texture;

		create_cube(
			tb,
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

/* TODO
void record_move_lights(
	vren::renderer const& renderer,
	int frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container
)
{
	// Move lights for the *first* light array buffer, then we reflect the changes to other N-1 light array buffers.

	m_move_lights->dispatch(
		frame_idx,
		cmd_buf,
		res_container,
		renderer.m_lights_array_descriptor_sets.at(0).get()
	);

	for (int i = 1; i < VREN_MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkBufferCopy buf_cpy{
			.srcOffset = 0,
			.dstOffset = 0,
			.size      = VK_WHOLE_SIZE,
		};

		vkCmdCopyBuffer(
			cmd_buf,
			renderer.m_point_lights_buffers[0].m_buffer.m_handle,
			renderer.m_point_lights_buffers[i].m_buffer.m_handle,
			1,
			&buf_cpy
		);
	}
}*/

void launch()
{
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

	auto ctx = vren::context::create(ctx_info);
	auto toolbox = vren::vk_utils::toolbox::create(ctx);

	/* renderer */
	auto renderer = vren::renderer::create(toolbox);
	renderer->m_clear_color = { 0.7f, 0.7f, 0.7f, 1.0f };

	/* UI renderer */
	auto ui_renderer = vren::imgui_renderer(toolbox, g_window);

	vren::render_list render_list;
	vren::light_array light_array;

	auto move_lights = vren_demo::move_lights::create(*renderer);

	/* Create surface */
	VkSurfaceKHR surf_handle;
	vren::vk_utils::check(glfwCreateWindowSurface(ctx->m_instance, g_window, nullptr, &surf_handle));
	auto surf = std::make_shared<vren::vk_surface_khr>(ctx, surf_handle);

	vren::presenter presenter(toolbox, surf);

	auto ui = vren_demo::ui::main_ui(ctx, toolbox, renderer);

	vren::profiler profiler(ctx, VREN_MAX_FRAMES_IN_FLIGHT * vren_demo::profile_slot::count);

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
			presenter.recreate_swapchain(cur_fb_width, cur_fb_height, renderer->m_render_pass);

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

		VkBufferMemoryBarrier mem_bar;

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

			/* Move lights */
			move_lights->dispatch(
				frame_idx,
				cmd_buf,
				res_container,
				{
					.m_scene_min = ui.m_scene_ui.m_scene_min,
					.m_scene_max = ui.m_scene_ui.m_scene_max,
					.m_speed     = ui.m_scene_ui.m_speed
				}
			);

			mem_bar = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = renderer->m_point_lights_buffers.at(0).m_buffer.m_handle,
				.offset = 0,
				.size = VK_WHOLE_SIZE
			};
			vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 1, &mem_bar, 0, nullptr);

			for (int i = 1; i < VREN_MAX_FRAMES_IN_FLIGHT; i++)
			{
				VkBufferCopy buf_cpy{
					.srcOffset = 0,
					.dstOffset = 0,
					.size      = VREN_POINT_LIGHTS_BUFFER_SIZE
				};

				vkCmdCopyBuffer(
					cmd_buf,
					renderer->m_point_lights_buffers.at(0).m_buffer.m_handle,
					renderer->m_point_lights_buffers.at(i).m_buffer.m_handle,
					1,
					&buf_cpy
				);
			}

			mem_bar = {
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = renderer->m_point_lights_buffers.at(frame_idx).m_buffer.m_handle,
				.offset = 0,
				.size = VK_WHOLE_SIZE
			};
			vkCmdPipelineBarrier(
				cmd_buf,
				VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				NULL,
				0, nullptr,
				1, &mem_bar,
				0, nullptr
			);

			/* Renders the scene */
			profiler.profile(prof_slot + vren_demo::profile_slot::MainPass, cmd_buf, res_container, [&]()
			{
				auto cam_data = vren::camera{
					.m_position = camera.m_position,
					.m_view = camera.get_view(),
					.m_projection = camera.get_projection()
				};

				renderer->render(
					frame_idx,
					cmd_buf,
					res_container,
					target,
					render_list,
					light_array,
					cam_data
				);
			});

			/* Renders the UI */
			profiler.profile(prof_slot + vren_demo::profile_slot::UiPass, cmd_buf, res_container, [&]()
			{
				ui_renderer.render(frame_idx, cmd_buf, res_container, target, [&]()
				{
					ui.m_fps_ui.notify_frame_profiling_data(prof_info);

					ui.show(render_list, light_array);
				});
			});
		});
	}

	vkDeviceWaitIdle(ctx->m_device);

	//glfwDestroyWindow(g_window);
	//glfwTerminate();
}

int main(int argc, char* argv[])
{
	launch();

	return 0;
}
