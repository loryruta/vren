#pragma once

#include <functional>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "renderer.hpp"
#include "command_pool.hpp"

namespace vren
{
	class gui
	{
	public:
		virtual void show() = 0;
	};

	class imgui_renderer
	{
	private:
		std::shared_ptr<vren::context> m_context;
		GLFWwindow* m_window; // for hooking events

		VkDescriptorPool m_descriptor_pool;
		VkRenderPass m_render_pass;

		void _init_descriptor_pool();
		void _init_render_pass();

	public:
		explicit imgui_renderer(std::shared_ptr<vren::context> const& context, GLFWwindow* window);
		~imgui_renderer();

		void record_commands(
			vren::frame& frame,
			vren::vk_command_buffer const& cmd_buf,
			vren::renderer_target const& target,
			std::function<void()> const& show_guis_func
		);
	};
}
