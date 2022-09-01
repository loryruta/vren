#pragma once

#include <cstdint>
#include <vector>

#include "vk_helpers/buffer.hpp"
#include "render_graph.hpp"
#include "render_target.hpp"
#include "model/basic_model_draw_buffer.hpp"
#include "gbuffer.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "gpu_repr.hpp"

namespace vren
{
	// Forward decl
	class context;

	class basic_renderer
	{
	private:
		vren::context const* m_context;

		vren::pipeline m_pipeline;

	public:
		explicit basic_renderer(vren::context const& context);

	private:
		vren::vk_render_pass create_render_pass();
		vren::pipeline create_graphics_pipeline();

	public:
		vren::render_graph_t render(
			vren::render_graph_allocator& render_graph_allocator,
			glm::uvec2 const& screen,
			vren::camera const& camera,
			vren::light_array const& light_array,
			vren::basic_model_draw_buffer const& draw_buffer,
			vren::gbuffer const& gbuffer,
			vren::vk_utils::depth_buffer_t const& depth_buffer
		);
	};
}
