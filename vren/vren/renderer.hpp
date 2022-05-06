#pragma once

#include <memory>

#include <volk.h>
#include <glm/glm.hpp>

#include "config.hpp"
#include "light.hpp"
#include "base/resource_container.hpp"
#include "mesh_shader_draw_pass.hpp"
#include "vertex_pipeline_draw_pass.hpp"
#include "vk_helpers/image.hpp"
#include "render_graph.hpp"

namespace vren
{
	// Forward decl
	class context;

	class basic_renderer;
	class depth_buffer_pyramid;

	// ------------------------------------------------------------------------------------------------
	// Render target
	// ------------------------------------------------------------------------------------------------

	struct render_target
	{
		vren::vk_utils::color_buffer_t const* m_color_buffer;
		vren::vk_utils::depth_buffer_t const* m_depth_buffer;
		VkRect2D m_render_area;
		VkViewport m_viewport;
		VkRect2D m_scissor;

		inline static render_target cover(uint32_t image_width, uint32_t image_height ,vren::vk_utils::color_buffer_t const& color_buffer, vren::vk_utils::depth_buffer_t const& depth_buffer)
		{
			return {
				.m_color_buffer = &color_buffer,
				.m_depth_buffer = &depth_buffer,
				.m_render_area = {
					.offset = {0, 0},
					.extent = {image_width, image_height}
				},
				.m_viewport = {
					.x = 0,
					.y = (float) image_height,
					.width = (float) image_width,
					.height = -((float) image_height),
					.minDepth = 0.0f,
					.maxDepth = 1.0f
				},
				.m_scissor {
					.offset = {0,0},
					.extent = {0,0}
				}
			};
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Basic renderer
	// ------------------------------------------------------------------------------------------------

	struct basic_renderer_draw_buffer
	{
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

	void set_object_names(vren::context const& context, vren::basic_renderer_draw_buffer const& draw_buffer);

	class basic_renderer
	{
	private:
		vren::context const* m_context;

	private:
		vren::vertex_pipeline_draw_pass m_vertex_pipeline_draw_pass;

	public:
		explicit basic_renderer(vren::context const& context);

		vren::render_graph::graph_t render(
			vren::render_graph::allocator& render_graph_allocator,
			vren::render_target const& render_target,
			vren::camera const& camera,
			vren::light_array const& light_array,
			vren::basic_renderer_draw_buffer const& draw_buffer
		);
	};

	// ------------------------------------------------------------------------------------------------
	// Mesh shader renderer
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
			vren::mesh_shader_renderer_draw_buffer const& draw_buffer
		);
	};
}
