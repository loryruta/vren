#pragma once

#include <memory>
#include <vector>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "base/resource_container.hpp"
#include "light.hpp"
#include "gpu_repr.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
	// Forward decl
	struct mesh_shader_renderer_draw_buffer;

	// ------------------------------------------------------------------------------------------------
	// Mesh shader draw pass
	// ------------------------------------------------------------------------------------------------

	class mesh_shader_draw_pass
	{
	private:
		vren::context const* m_context;
		VkRenderPass m_render_pass;
		uint32_t m_subpass_idx;
		vren::vk_utils::pipeline m_pipeline;

	public:
		mesh_shader_draw_pass(vren::context const& context, VkRenderPass render_pass, uint32_t subpass_idx);

	private:
		vren::vk_utils::pipeline create_graphics_pipeline();

	public:
		void render(
			uint32_t descriptor_set,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			vren::camera const& camera,
			vren::mesh_shader_renderer_draw_buffer const& draw_buffer,
			vren::light_array const& light_array
		);
	};
}

