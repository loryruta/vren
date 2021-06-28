
#include "renderer.hpp"
#include "presenter.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>

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

int main(int argc, char* argv[])
{
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("Couldn't initialize GLFW.");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(500, 500, "VRen Example", nullptr, nullptr);
	if (window == nullptr) {
		throw std::runtime_error("Couldn't create the window.");
	}

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

	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(renderer.m_instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface.");
	}

	vren::presenter_info presenter_info{};
	presenter_info.m_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	vren::presenter presenter(renderer, presenter_info, surface, {500, 500});

	vren::render_list render_list(renderer);

	vren::render_object& cube = render_list.create_render_object();
	create_cube(cube);

	vren::camera camera{};
	camera.m_projection_matrix = glm::identity<glm::mat4>();
	camera.m_view_matrix = glm::identity<glm::mat4>();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		presenter.present(render_list, camera);
	}

	// todo destroy stuff

	return 0;
}
