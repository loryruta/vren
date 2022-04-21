#pragma once

#include "vk_helpers/shader.hpp"
#include "vk_helpers/buffer.hpp"
#include "light.hpp"
#include "gpu_repr.hpp"

namespace vren
{
	// Forward decl
	class context;
	class basic_renderer_draw_buffer;

	//

	class vertex_pipeline_draw_pass
	{
	public:
		static constexpr uint32_t k_texture_buffer_descriptor_set_idx = 0;
		static constexpr uint32_t k_material_buffer_descriptor_set_idx = 1;

	private:
		vren::context const* m_context;
		vren::vk_utils::pipeline m_pipeline;

	public:
		vertex_pipeline_draw_pass(vren::context const& context, VkRenderPass render_pass, uint32_t subpass_idx);

	private:
		vren::vk_utils::pipeline create_graphics_pipeline(VkRenderPass render_pass, uint32_t subpass_idx);

	public:
		void draw(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			vren::camera const& camera,
			vren::basic_renderer_draw_buffer const& draw_buffer,
			vren::light_array const& light_array
		);
	};
}
