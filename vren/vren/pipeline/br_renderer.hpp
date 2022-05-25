#pragma once

#include <cstdint>
#include <vector>

#include "vk_helpers/buffer.hpp"
#include "render_graph.hpp"
#include "vertex_pipeline_draw_pass.hpp"
#include "render_target.hpp"

namespace vren::br
{
	// Forward decl
	class context;

	struct draw_buffer
	{
		std::string m_name = "unnamed";

		struct mesh
		{
			uint32_t m_vertex_offset, m_vertex_count;
			uint32_t m_index_offset, m_index_count;
			uint32_t m_instance_offset, m_instance_count;
			uint32_t m_material_idx;
		};

		vren::vk_utils::buffer m_vertex_buffer;
		vren::vk_utils::buffer m_index_buffer;
		vren::vk_utils::buffer m_instance_buffer;
		std::vector<mesh> m_meshes;
	};

	void set_object_names(vren::context const& context, vren::br::draw_buffer const& draw_buffer);

	class renderer
	{
	private:
		vren::context const* m_context;

	private:
		vren::vertex_pipeline_draw_pass m_vertex_pipeline_draw_pass;

	public:
		explicit renderer(vren::context const& context);

		vren::render_graph::graph_t render(
			vren::render_graph::allocator& render_graph_allocator,
			vren::render_target const& render_target,
			vren::camera const& camera,
			vren::light_array const& light_array,
			vren::basic_renderer_draw_buffer const& draw_buffer
		);
	};
}
