#pragma once

#include <memory>

#include <volk.h>
#include <glm/glm.hpp>

#include "config.hpp"
#include "light.hpp"
#include "base/resource_container.hpp"
#include "mesh_shader_draw_pass.hpp"
#include "vertex_pipeline_draw_pass.hpp"

namespace vren
{
	// Forward decl
	class context;

	class basic_renderer;

	// ------------------------------------------------------------------------------------------------

	struct render_target
	{
		VkFramebuffer m_framebuffer;
		VkRect2D m_render_area;
		VkViewport m_viewport;
		VkRect2D m_scissor;
	};

	// ------------------------------------------------------------------------------------------------
	// Basic renderer
	// ------------------------------------------------------------------------------------------------

	class basic_renderer_draw_buffer
	{
	public:
		struct mesh
		{
			uint32_t m_vertex_offset, m_vertex_count;
			uint32_t m_index_offset, m_index_count;
			uint32_t m_instance_offset, m_instance_count;
			uint32_t m_material_idx;
		};

	public:
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

	public:
		vren::vk_render_pass m_render_pass;

	private:
		vren::vertex_pipeline_draw_pass m_vertex_pipeline_draw_pass;

		vren::vk_render_pass create_render_pass();

	public:
		explicit basic_renderer(vren::context const& context);

		void render(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
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

	public:
		vren::vk_render_pass m_render_pass;

	private:
		vren::mesh_shader_draw_pass m_mesh_shader_draw_pass;

		vren::vk_render_pass create_render_pass();

	public:
		explicit mesh_shader_renderer(vren::context const& context);

		void render(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			vren::render_target const& render_target,
			vren::camera const& camera,
			vren::light_array const& light_array,
			vren::mesh_shader_renderer_draw_buffer const& draw_buffer
		);
	};
}
