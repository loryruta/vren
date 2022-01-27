#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "context.hpp"
#include "render_list.hpp"
#include "light.hpp"
#include "frame.hpp"

namespace vren
{
	// Forward decl
	class simple_draw_pass;
	class debug_gui;

	struct camera
	{
		glm::vec3 m_position;
		float _pad;
		glm::mat4 m_view;
		glm::mat4 m_projection;
	};

	struct renderer_target
	{
		VkFramebuffer m_framebuffer;
		VkRect2D m_render_area;
		VkViewport m_viewport;
		VkRect2D m_scissor;
	};

	class renderer : public std::enable_shared_from_this<renderer>
	{
	public:
		static VkFormat const k_color_output_format = VK_FORMAT_B8G8R8A8_UNORM;

		std::shared_ptr<context> m_context;

		VkRenderPass m_render_pass;
		VkClearColorValue m_clear_color = {1.0f, 0.0f, 0.0f, 1.0f};

		std::unique_ptr<vren::debug_gui> m_debug_gui;
		std::unique_ptr<vren::simple_draw_pass> m_simple_draw_pass;

	private:
		renderer(std::shared_ptr<context> const& ctx);

		void _init();
		void _init_render_pass();

	public:
		~renderer();

		void render(
			vren::frame& frame,
			vren::renderer_target const& target,
			vren::render_list const& render_list,
			vren::lights_array const& lights_array,
			vren::camera const& camera,
			std::vector<VkSemaphore> const& wait_semaphores = {},
			VkFence signal_fence = VK_NULL_HANDLE
		);

		static std::shared_ptr<vren::renderer> create(std::shared_ptr<context> const& ctx);
	};
}
