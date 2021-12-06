
#include "renderer.hpp"
#include "presenter.hpp"

#include "camera.hpp"
#include "ai_scene_loader.hpp"

#include <iostream>
#include <optional>
#include <stdio.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define WINDOW_WIDTH  500
#define WINDOW_HEIGHT 500

GLFWwindow* g_window;

void create_cube(vren::render_object& render_object)
{
	std::vector<vren::vertex> vertices = {
		vren::vertex{ .m_position = { -1, -1, -1 } },
		vren::vertex{ .m_position = { 1, -1, -1 } },
		vren::vertex{ .m_position = { 1, 1, -1 } },
		vren::vertex{ .m_position = { -1, 1, -1 } },
		vren::vertex{ .m_position = { -1, -1, 1 } },
		vren::vertex{ .m_position = { 1, -1, 1 } },
		vren::vertex{ .m_position = { 1, 1, 1 } },
		vren::vertex{ .m_position = { -1, 1, 1 } }
	};
	render_object.set_vertex_data(vertices.data(), vertices.size());

	std::vector<uint32_t> indices = {
		0, 1, 3, 3, 1, 2,
		1, 5, 2, 2, 5, 6,
		5, 4, 6, 6, 4, 7,
		4, 0, 7, 7, 0, 3,
		3, 2, 7, 7, 2, 6,
		4, 5, 0, 0, 5, 1
	};
	render_object.set_indices_data(indices.data(), indices.size());

	std::initializer_list<vren::instance_data> instances = {
		vren::instance_data{}
	};
	render_object.set_instances_data(instances.begin(), instances.size());
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
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("Couldn't initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	g_window = glfwCreateWindow(500, 500, "VRen Example", nullptr, nullptr);
	if (g_window == nullptr) {
		throw std::runtime_error("Couldn't create the window.");
	}

	glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// ---------------------------------------------------------------- Context initialization

	vren::renderer_info renderer_info;
	renderer_info.m_app_name = "vren_example";
	renderer_info.m_app_version = VK_MAKE_VERSION(1, 0, 0);
	renderer_info.m_layers = {};

	uint32_t glfw_extensions_count = 0;
	char const** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extensions_count);
	for (int i = 0; i < glfw_extensions_count; i++) {
		renderer_info.m_extensions.push_back(glfw_extensions[i]);
	}

	renderer_info.m_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	vren::renderer renderer(renderer_info);

	renderer.m_clear_color = { 0.7f, 0.7f, 0.7f, 1.0f };

	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(renderer.m_instance, g_window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface.");
	}

	vren::presenter_info presenter_info{};
	presenter_info.m_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	vren::presenter presenter(renderer, presenter_info, surface, {500, 500});

	vren::render_list render_list(renderer);

	// ---------------------------------------------------------------- Creation of a test cube

	//auto& cube = render_list.create_render_object();
	//create_cube(cube); // Baking

	// ---------------------------------------------------------------- Creation of the scene

	// Scene loading
	char const* scene_path = "resources/models/skull/12140_Skull_v3_L2.obj";

	printf("Loading scene: %s\n", scene_path);
	fflush(stdout);

	Assimp::Importer importer;
	aiScene const* ai_scene = importer.ReadFile(scene_path,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		//aiProcess_GenNormals |
		//aiProcess_CalcTangentSpace |
		aiProcess_EmbedTextures |
		aiProcess_FlipUVs
	);

	if (!ai_scene)
	{
		fprintf(stderr, "Couldn't load scene at: %s\n", scene_path);
		fflush(stderr);

		return 1;
	}

	printf("Baking scene\n");
	fflush(stdout);

	vren_demo::ai_scene_baker scene_baker(renderer);
	scene_baker.bake(ai_scene, render_list);

	// ---------------------------------------------------------------- Game loop

	vren_demo::camera camera{};
	camera.m_aspect_ratio = WINDOW_WIDTH / (float) WINDOW_HEIGHT;

	float last_time = -1.0;
	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		auto cur_time = (float) glfwGetTime();
		float dt = last_time >= 0 ? (cur_time - last_time) : 0;
		last_time = cur_time;

		if (glfwGetKey(g_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(g_window, GLFW_TRUE);
		}

		update_camera(dt, camera);

		presenter.present(render_list, { .m_view = camera.get_view(), .m_projection = camera.get_projection() });
	}

	// todo destroy stuff

	return 0;
}
