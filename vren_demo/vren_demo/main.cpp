#include <memory>
#include <numeric>

#include <volk.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <meshoptimizer.h>

#include "vren/log.hpp"

#include "app.hpp"

void initialize_point_lights_direction(std::span<glm::vec3> point_lights_dir)
{
	for (size_t i = 0; i < point_lights_dir.size(); i++) {
		point_lights_dir[i] = glm::ballRand(1.0f);
	}
}

void move_and_bounce_point_lights(std::span<vren::point_light> point_lights, std::span<glm::vec3> point_lights_dir, glm::vec3 const& sm, glm::vec3 const& sM, float speed, float dt)
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
		app->on_frame();
	}

	app.reset();

	VREN_INFO("Bye bye!\n");

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
