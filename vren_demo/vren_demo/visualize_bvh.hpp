#pragma once

#include "pipeline/render_graph.hpp"
#include "pipeline/render_target.hpp"
#include "pipeline/debug_renderer.hpp"
#include "vk_helpers/buffer.hpp"

namespace vren_demo
{
    class visualize_bvh
    {
	private:
		vren::context const* m_context;

		vren::pipeline m_pipeline;

	public:
		visualize_bvh(vren::context const& context);

		vren::render_graph_t write(
			vren::render_graph_allocator& render_graph_allocator,
			vren::vk_utils::buffer const& bvh,
			uint32_t level_count,
			vren::debug_renderer_draw_buffer const& draw_buffer
		);
    };
}
