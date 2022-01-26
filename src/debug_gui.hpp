#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "frame.hpp"

namespace vren
{
	class renderer; // forward decl
	class presenter;

	class debug_gui
	{
	private:
		vren::renderer& m_renderer;
		GLFWwindow* m_window; // for hooking events

		VkDescriptorPool m_descriptor_pool;

	public:
		explicit debug_gui(vren::renderer& renderer, GLFWwindow* window);
		~debug_gui();

		void render(vren::frame& frame);
	};

}
