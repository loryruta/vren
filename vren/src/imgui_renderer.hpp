#pragma once

#include <memory>
#include <functional>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "utils/vk_toolbox.hpp"
#include "resource_container.hpp"
#include "renderer.hpp"

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
		std::shared_ptr<vren::vk_utils::toolbox> m_toolbox;
		GLFWwindow* m_window;

		explicit imgui_renderer(std::shared_ptr<vren::vk_utils::toolbox> const& tb, GLFWwindow* window);
		~imgui_renderer();

		void render(
			int frame_idx,
			VkCommandBuffer cmd_buf,
            vren::resource_container& res_container,
			vren::render_target const& target,
			std::function<void()> const& show_guis_func
		);
	};
}
