#pragma once

#include <memory>
#include <functional>

#include "base/resource_container.hpp"
#include "renderer.hpp"
#include "render_graph.hpp"

namespace vren
{
	// Forward decl
	class context;

	// --------------------------------------------------------------------------------------------------------------------------------

	struct imgui_windowing_backend_hooks
	{
		std::function<void()> m_init_callback;
		std::function<void()> m_new_frame_callback;
		std::function<void()> m_shutdown_callback;
	};

	class imgui_renderer
	{
	private:
		vren::context const* m_context;
		imgui_windowing_backend_hooks m_windowing_backend_hooks;

		vren::vk_descriptor_pool m_descriptor_pool;
		vren::vk_render_pass m_render_pass;

	public:
		imgui_renderer(vren::context const& context, imgui_windowing_backend_hooks const& windowing_backend_hooks);
		~imgui_renderer();

	private:
		vren::vk_descriptor_pool create_descriptor_pool();
		vren::vk_render_pass create_render_pass();

	public:
		vren::render_graph::graph_t render(
			vren::render_graph::allocator& allocator,
			vren::render_target const& render_target,
			std::function<void()> const& show_ui_callback
		);
	};
}
