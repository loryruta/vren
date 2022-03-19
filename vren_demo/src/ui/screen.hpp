#pragma once

#include <optional>

#include <vulkan/vulkan.h>

#include "imgui_renderer.hpp"
#include "utils/vk_raii.hpp"
#include "utils/image.hpp"

namespace vren_demo
{
	class screen
	{
	private:
		std::shared_ptr<vren::context> m_context;
		vren::vk_utils::custom_framebuffer m_framebuffer;

		std::optional<ImVec2> m_current_size;

		void _recreate_framebuffer(ImVec2 const& size);

	public:
		screen(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::vk_render_pass> const& render_pass,
			ImVec2 const& init_size
		);
		~screen();

		void show(ImVec2 const& size, std::function<void(vren::render_target const&)> const& render_func);
	};
}
