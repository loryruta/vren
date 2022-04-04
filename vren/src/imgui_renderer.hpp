#pragma once

#include <memory>
#include <functional>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "base/resource_container.hpp"
#include "renderer.hpp"

namespace vren
{
	// Forward decl
	class context;

	// --------------------------------------------------------------------------------------------------------------------------------
	// ImGui renderer
	// --------------------------------------------------------------------------------------------------------------------------------

	class imgui_renderer
	{
	private:
		vren::context const* m_context;

		VkDescriptorPool m_descriptor_pool;
		VkRenderPass m_render_pass;

		void init_render_pass();

	public:
		GLFWwindow* m_window;

		explicit imgui_renderer(vren::context const& ctx, GLFWwindow* window);
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
