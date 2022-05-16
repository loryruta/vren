#pragma once

#include "vk_helpers/buffer.hpp"
#include "mesh_shader_draw_pass.hpp"
#include "renderer.hpp" // For render_target

namespace vren
{
	class context;

	// ------------------------------------------------------------------------------------------------
	// Mesh-shader renderer
	// ------------------------------------------------------------------------------------------------

	struct mesh_shader_renderer_draw_buffer
	{
		vren::vk_utils::buffer m_vertex_buffer;
		vren::vk_utils::buffer m_meshlet_vertex_buffer;
		vren::vk_utils::buffer m_meshlet_triangle_buffer;
		vren::vk_utils::buffer m_meshlet_buffer;
		vren::vk_utils::buffer m_instanced_meshlet_buffer;
		size_t m_instanced_meshlet_count;
		vren::vk_utils::buffer m_instance_buffer;
	};

	void set_object_names(vren::context const& context, vren::mesh_shader_renderer_draw_buffer const& draw_buffer);

	class mesh_shader_renderer
	{
	private:
		vren::context const* m_context;
		vren::mesh_shader_draw_pass m_mesh_shader_draw_pass;

	public:
		explicit mesh_shader_renderer(vren::context const& context);

		vren::render_graph::graph_t render(
			vren::render_graph::allocator& render_graph_allocator,
			vren::render_target const& render_target,
			vren::camera const& camera,
			vren::light_array const& light_array,
			vren::mesh_shader_renderer_draw_buffer const& draw_buffer,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid
		);
	};
}
