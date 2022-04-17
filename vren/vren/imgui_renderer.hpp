#pragma once

#include <memory>
#include <functional>

#include <volk.h>
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

		vren::vk_render_pass create_render_pass();

	public:
		vren::vk_render_pass m_render_pass;

		GLFWwindow* m_window;

		explicit imgui_renderer(vren::context const& ctx, GLFWwindow* window);
		~imgui_renderer();

		void render(
			uint32_t frame_idx,
			VkCommandBuffer cmd_buf,
            vren::resource_container& res_container,
			vren::render_target const& target,
			std::function<void()> const& show_guis_func
		);
	};
}
