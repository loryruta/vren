#pragma once

#include <cstdint>
#include <vector>

#include "vk_helpers/buffer.hpp"
#include "render_graph.hpp"
#include "vertex_pipeline_draw_pass.hpp"
#include "render_target.hpp"
#include "model/basic_model_draw_buffer.hpp"
#include "gbuffer.hpp"
#include "camera.hpp"

namespace vren
{
	// Forward decl
	class context;

	class basic_renderer
	{
	private:
		vren::context const* m_context;

	private:
		vren::vertex_pipeline_draw_pass m_vertex_pipeline_draw_pass;

	public:
		explicit basic_renderer(vren::context const& context);

		vren::render_graph_t render(
			vren::render_graph_allocator& render_graph_allocator,
			glm::uvec2 const& screen,
			vren::camera_data const& camera_data,
			vren::light_array const& light_array,
			vren::basic_model_draw_buffer const& draw_buffer,
			vren::gbuffer const& gbuffer,
			vren::vk_utils::depth_buffer_t const& depth_buffer
		);
	};
}
