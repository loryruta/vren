#include <memory>
#include <numeric>

#include <volk.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <meshoptimizer.h>

#include <vren/log.hpp>

#include "app.hpp"

void glfw_error_callback(int error_code, const char* description)
{
	VREN_ERROR("GLFW error (code {}): {}\n", error_code, description);
}

int main(int argc, char* argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0); // Disable stdout/stderr buffering
	setvbuf(stderr, NULL, _IONBF, 0);

	// Command-line arguments processing
	if (argc != (1 + 1))
	{
		VREN_ERROR("Invalid usage: <scene_filepath>\n");
		exit(1);
	}
	argv++;

	// GLFW init
	glfwSetErrorCallback(glfw_error_callback);

	if (glfwInit() != GLFW_TRUE)
	{
		VREN_ERROR("Failed to initialize GLFW\n");
		exit(1);
	}

	// GLFW window creation
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

	// App creation
	auto app = std::make_unique<vren_demo::app>(window); // Dynamically allocated not to exceed stack allocation limits

	// Scene loading
	app->load_scene(argv[0]);

	// Gameloop
	VREN_INFO("Gameloop\n");

	int last_framebuffer_width = -1, last_framebuffer_height = -1;
	float last_time = -1.0f;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Handle window resize
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

		if (framebuffer_width != last_framebuffer_width || framebuffer_height != last_framebuffer_height)
		{
			app->on_window_resize(framebuffer_width, framebuffer_height);

			last_framebuffer_width = framebuffer_width;
			last_framebuffer_height = framebuffer_height;
		}

		// Update logic
		float current_time = (float) glfwGetTime();
		float dt = last_time >= 0 ? (current_time - last_time) : 0;
		last_time = current_time;

		app->on_update(dt);

		// Draw and present frame
		app->on_frame(dt);
	}

	app.reset();

	VREN_INFO("Bye bye!\n");

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
