#pragma once

#include "vk_helpers/buffer.hpp"
#include "mesh_shader_draw_pass.hpp"
#include "render_target.hpp"
#include "render_graph.hpp"
#include "model/clusterized_model_draw_buffer.hpp"

namespace vren
{
	class context;

	// ------------------------------------------------------------------------------------------------
	// Mesh Shader Renderer
	// ------------------------------------------------------------------------------------------------

	class mesh_shader_renderer
	{
	private:
		vren::context const* m_context;
		vren::mesh_shader_draw_pass m_mesh_shader_draw_pass;

		bool m_occlusion_culling;

	public:
		explicit mesh_shader_renderer(
			vren::context const& context,
			bool occlusion_culling
		);

		inline bool is_occlusion_culling_enabled() const
		{
			return m_occlusion_culling;
		}

		vren::render_graph_t render(
			vren::render_graph_allocator& render_graph_allocator,
			vren::render_target const& render_target,
			vren::camera const& camera,
			vren::light_array const& light_array,
			vren::clusterized_model_draw_buffer const& draw_buffer,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid
		);
	};
}
