#pragma once

#include <memory>
#include <functional>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "renderer.hpp"
#include "pooling/command_pool.hpp"
#include "pooling/descriptor_pool.hpp"

namespace vren
{
	// --------------------------------------------------------------------------------------------------------------------------------
	// imgui_renderer
	// --------------------------------------------------------------------------------------------------------------------------------

	class imgui_renderer
	{
	private:
		VkDescriptorPool m_descriptor_pool;
		VkRenderPass m_render_pass;

		void _init_render_pass();

	public:
		std::shared_ptr<vren::context> m_context;
		GLFWwindow* m_window;

		explicit imgui_renderer(std::shared_ptr<vren::context> const& context, GLFWwindow* window);
		~imgui_renderer();

		void render(
			int frame_idx,
			vren::command_graph& cmd_graph,
            vren::resource_container& res_container,
			vren::render_target const& target,
			std::function<void()> const& show_guis_func
		);
	};
}
