#pragma once

#include <optional>

#include <vulkan/vulkan.h>

#include "utils/vk_raii.hpp"
#include "imgui_renderer.hpp"

namespace vren_demo
{
	class screen
	{
	private:
		VkRenderPass m_render_pass;
		std::shared_ptr<vren::imgui_renderer> m_imgui_renderer;

		std::optional<ImVec2> m_current_size;

		std::shared_ptr<color_buffer> m_color_buffer;
		std::shared_ptr<depth_buffer> m_depth_buffer;
		std::shared_ptr<vren::vk_framebuffer> m_framebuffer;

		void _recreate_framebuffer(ImVec2 const& size);

	public:
		/** A single descriptor set that could be referenced by all the parallel frames,
		 * when the window is resized, it gets destroyed and recreated. */
		std::shared_ptr<vren::vk_descriptor_set> m_descriptor_set;

		screen(
			std::shared_ptr<vren::imgui_renderer> const& ctx,
			VkRenderPass render_pass
		);
		~screen();

		void show(ImVec2 const& size, std::function<void(vren::render_target const&)> const& render_func);
	};
}
