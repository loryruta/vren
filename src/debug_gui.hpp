#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "renderer.hpp"

namespace vren
{
	class debug_gui
	{
	private:
		std::shared_ptr<vren::renderer> m_renderer;
		GLFWwindow* m_window; // for hooking events

		VkDescriptorPool m_descriptor_pool;

	public:
		explicit debug_gui(std::shared_ptr<vren::renderer> const& renderer, GLFWwindow* window);
		~debug_gui();

		void render(vren::frame& frame);
	};

}
