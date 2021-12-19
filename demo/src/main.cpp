
#include "renderer.hpp"
#include "presenter.hpp"

#include "camera.hpp"
#include "ai_scene_loader.hpp"

#include <iostream>
#include <optional>
#include <numeric>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define VREN_DEMO_WINDOW_WIDTH  1024
#define VREN_DEMO_WINDOW_HEIGHT 720

GLFWwindow* g_window;

void create_cube(
	vren::render_object& render_obj,
	glm::vec3 position,
	glm::vec3 rotation,
	glm::vec3 scale,
	vren::material* material
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
	render_obj.set_vertices_data(vertices.data(), vertices.size());

	std::vector<uint32_t> indices(vertices.size());
	std::iota(indices.begin(), indices.end(), 0);
	render_obj.set_indices_data(indices.data(), indices.size());

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

	vren::instance_data inst_data{};
	inst_data.m_transform = transf;
	render_obj.set_instances_data(&inst_data, 1);

	render_obj.m_material = material;
}

void create_light(
	vren::renderer& renderer,
	vren::render_list* render_list,
	vren::lights_array* lights_array,
	glm::vec3 position,
	glm::vec3 color
)
{
	auto& render_obj = render_list->create_render_object();

	auto mat = renderer.m_material_manager->create_material();
	mat->m_albedo_texture = renderer.m_blue_texture;
	mat->m_roughness = 1.0f;
	mat->m_metallic = 0.0f;

	create_cube(
		render_obj,
		position,
		glm::vec3(0),
		glm::vec3(1),
		mat
	);

	auto& light = lights_array->create_point_light().first.get();
	light.m_position = position;
	light.m_color    = color;
}

void create_cube_scene(vren::renderer& renderer, vren::render_list* render_list, vren::lights_array* lights_arr)
{
	float const surface_y = 1;
	float const surface_side = 30;
	float const r = 10;
	int const n = 50;

	{ // Surface
		auto& surface = render_list->create_render_object();

		auto mat = renderer.m_material_manager->create_material();
		mat->m_albedo_texture = renderer.m_green_texture;
		mat->m_metallic  = 0.5f;
		mat->m_roughness = 0.8f;
		surface.m_material = mat;

		create_cube(
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
		auto& cube = render_list->create_render_object();

		float cos_i = glm::cos(2 * glm::pi<float>() / (float) n * (float) i);
		float sin_i = glm::sin(2 * glm::pi<float>() / (float) n * (float) i);

		auto mat = renderer.m_material_manager->create_material();
		mat->m_albedo_texture = renderer.m_red_texture;
		mat->m_metallic  = 1.0f;
		mat->m_roughness = 0.2f; // TODO metallic = 1, roughness = 1 excluded

		create_cube(
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

	// Lights
	create_light(
		renderer,
		render_list,
		lights_arr,
		glm::vec3(0, 8, 0),
		glm::vec3(1, 1, 1)
	);
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

	// Scale
	// todo
}

int main(int argc, char* argv[])
{
	if (glfwInit() != GLFW_TRUE)
	{
		throw std::runtime_error("Couldn't initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	g_window = glfwCreateWindow(VREN_DEMO_WINDOW_WIDTH, VREN_DEMO_WINDOW_HEIGHT, "VRen", nullptr, nullptr);
	if (g_window == nullptr)
	{
		throw std::runtime_error("Couldn't create the window.");
	}

	glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// ---------------------------------------------------------------- Context initialization

	vren::renderer_info renderer_info;
	renderer_info.m_app_name = "vren_demo";
	renderer_info.m_app_version = VK_MAKE_VERSION(1, 0, 0);
	renderer_info.m_layers = {};

	uint32_t glfw_extensions_count = 0;
	char const** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
	for (int i = 0; i < glfw_extensions_count; i++) {
		renderer_info.m_extensions.push_back(glfw_extensions[i]);
	}

	renderer_info.m_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	auto renderer = std::make_shared<vren::renderer>(renderer_info);

	renderer->m_clear_color = { 0.7f, 0.7f, 0.7f, 1.0f };

	VkSurfaceKHR surface;
	vren::vk_utils::check(glfwCreateWindowSurface(renderer->m_instance, g_window, nullptr, &surface));

	vren::presenter_info presenter_info{};
	presenter_info.m_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	vren::presenter presenter(renderer, presenter_info, surface, {VREN_DEMO_WINDOW_WIDTH, VREN_DEMO_WINDOW_HEIGHT});

	auto render_list = renderer->create_render_list();
	auto lights_arr  = renderer->create_light_array();

	create_cube_scene(*renderer.get(), render_list, lights_arr);

	//render_list->update_device_buffers();
	lights_arr->update_device_buffers();
	renderer->m_material_manager->upload_device_buffer();

	// ---------------------------------------------------------------- Game loop

	vren_demo::camera camera{};
	camera.m_aspect_ratio = VREN_DEMO_WINDOW_WIDTH / (float) VREN_DEMO_WINDOW_HEIGHT;

	float last_time = -1.0;
	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		auto cur_time = (float) glfwGetTime();
		float dt = last_time >= 0 ? (cur_time - last_time) : 0;
		last_time = cur_time;

		if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(g_window, GLFW_TRUE);
		}

		update_camera(dt, camera);

		presenter.present(*render_list, *lights_arr, { .m_view = camera.get_view(), .m_projection = camera.get_projection() });
	}

	return 0;
}
