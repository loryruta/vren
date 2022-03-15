#include <memory>
#include <iostream>
#include <optional>
#include <numeric>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "context.hpp"
#include "presenter.hpp"
#include "utils/buffer.hpp"
#include "camera.hpp"
#include "tinygltf_loader.hpp"
#include "imgui_renderer.hpp"

#define VREN_DEMO_WINDOW_WIDTH  1280
#define VREN_DEMO_WINDOW_HEIGHT 720

GLFWwindow* g_window;

void create_cube(
	std::shared_ptr<vren::context> const& ctx,
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
			vren::vk_utils::create_vertex_buffer(ctx, vertices.data(), vertices.size())
		);
	render_obj.set_vertices_buffer(vertices_buf, vertices.size());

	// Indices
	std::vector<uint32_t> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);

	auto indices_buf =
		std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::create_indices_buffer(ctx, indices.data(), indices.size())
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
				vren::vk_utils::create_instances_buffer(ctx, &inst, 1)
			);
		render_obj.set_indices_buffer(instances_buf, 1);
	}


	render_obj.m_material = material;
}

void create_cube_scene(
	std::shared_ptr<vren::context> const& ctx,
	vren::render_list& render_list,
	vren::light_array& lights_arr
)
{
	float const surface_y = 1;
	float const surface_side = 30;
	float const r = 10;
	int const n = 50;

	{ // Surface
		auto& surface = render_list.create_render_object();

		auto mat = std::make_shared<vren::material>(ctx);
		mat->m_base_color_texture = ctx->m_green_texture;
		mat->m_metallic_roughness_texture = ctx->m_green_texture;
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
		mat->m_base_color_texture = ctx->m_red_texture;
		mat->m_metallic_roughness_texture = ctx->m_green_texture;

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

template<glm::length_t L, typename T>
void log_vec(glm::vec<L, T> v)
{
	for (int i = 0; i < L; i++) {
		std::cout << " v[" << i << "]: " << v[i];
	}
	std::cout << std::endl;
}

void update_camera(float dt, vren_demo::camera& camera)
{
	const glm::vec4 k_world_up = glm::vec4(0, 1, 0, 0);

	// Movement
	const float k_speed = 4.0f; // m/s

	glm::vec3 direction{};
	if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) direction += -camera.get_forward();
	if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) direction += camera.get_forward();
	if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) direction += -camera.get_right();
	if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) direction += camera.get_right();
	if (glfwGetKey(g_window, GLFW_KEY_SPACE) == GLFW_PRESS) direction += camera.get_up();
	if (glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) direction += -camera.get_up();

	float inc = k_speed * dt;
	if (glfwGetKey(g_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		inc *= 100;
	}

	camera.m_position += direction * inc;

	// Rotation
	static std::optional<glm::dvec2> last_cur_pos;
	const glm::vec2 k_sensitivity = glm::radians(glm::vec2(90.0f)); // rad/(pixel * s)

	glm::dvec2 cur_pos{};
	glfwGetCursorPos(g_window, &cur_pos.x, &cur_pos.y);

	if (last_cur_pos.has_value())
	{
		glm::vec2 d_cur_pos = glm::vec2(cur_pos - last_cur_pos.value()) * k_sensitivity * dt;
		camera.m_yaw += d_cur_pos.x;
		camera.m_pitch += d_cur_pos.y;
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

int main(int argc, char* argv[])
{
	if (glfwInit() != GLFW_TRUE)
	{
		throw std::runtime_error("Couldn't create GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

	auto renderer = vren::renderer::create(ctx);
	renderer->m_clear_color = { 0.7f, 0.7f, 0.7f, 1.0f };

	auto imgui_renderer = std::make_unique<vren::imgui_renderer>(ctx, g_window);

	VkSurfaceKHR surface;
	vren::vk_utils::check(glfwCreateWindowSurface(ctx->m_instance, g_window, nullptr, &surface));

	//vren::presenter_info presenter_info{};
	//presenter_info.m_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	vren::presenter presenter(ctx, surface);
    presenter.recreate_swapchain(VREN_DEMO_WINDOW_WIDTH, VREN_DEMO_WINDOW_HEIGHT, renderer->m_render_pass);

	auto render_list = vren::render_list::create(ctx);
	vren::light_array lights_arr{};

	char const* model_path = "resources/models/Sponza/glTF/Sponza.gltf";
	vren::tinygltf_scene loaded_scene;
	vren::tinygltf_loader gltf_loader(ctx);
	//gltf_loader.load_from_file(model_path, *render_list, loaded_scene);

	// ---------------------------------------------------------------- Game loop

	vren_demo::camera camera{};
	camera.m_aspect_ratio = VREN_DEMO_WINDOW_WIDTH / (float) VREN_DEMO_WINDOW_HEIGHT;

	glfwSetKeyCallback(g_window, on_key_press);

	float last_time = -1.0;
	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		auto cur_time = (float) glfwGetTime();
		float dt = last_time >= 0 ? (cur_time - last_time) : 0;
		last_time = cur_time;

		if (glfwGetInputMode(g_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
			update_camera(dt, camera);
		}

		int win_width, win_height;
		glfwGetWindowSize(g_window, &win_width, &win_height);

		presenter.present([&](int frame_idx, vren::resource_container& res_container, vren::render_target const& renderer_target, VkSemaphore src_sem, VkSemaphore dst_sem)
        {
            auto cam_data = vren::camera{
                .m_position = camera.m_position,
                .m_view = camera.get_view(),
                .m_projection = camera.get_projection()
            };
            renderer->render(frame_idx, res_container, renderer_target, src_sem, dst_sem, *render_list, lights_arr, cam_data);
        });
	}

	vkDeviceWaitIdle(ctx->m_device);

	return 0;
}
