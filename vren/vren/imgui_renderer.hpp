#pragma once

#include <memory>
#include <functional>

#include <volk.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "base/resource_container.hpp"
#include "renderer.hpp"
#include "render_graph.hpp"

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
		GLFWwindow* m_window;

		vren::vk_render_pass m_render_pass;

		VkDescriptorPool m_descriptor_pool;

		vren::vk_render_pass create_render_pass();

	public:
		explicit imgui_renderer(vren::context const& context, GLFWwindow* window);
		~imgui_renderer();

		vren::render_graph_node* render(vren::render_target const& render_target, std::function<void()> const& show_ui_callback);
	};
}
