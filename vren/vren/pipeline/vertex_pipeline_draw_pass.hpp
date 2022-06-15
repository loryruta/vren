#pragma once

#include "vk_helpers/shader.hpp"
#include "vk_helpers/buffer.hpp"
#include "light.hpp"
#include "gpu_repr.hpp"
#include "model/basic_model_draw_buffer.hpp"

namespace vren
{
	// Forward decl
	class context;

	class vertex_pipeline_draw_pass
	{
	private:
		vren::context const* m_context;
		vren::pipeline m_pipeline;

	public:
		vertex_pipeline_draw_pass(vren::context const& context);

	private:
		vren::pipeline create_graphics_pipeline();

	public:
		void draw(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			vren::camera const& camera,
			vren::basic_model_draw_buffer const& draw_buffer,
			vren::light_array const& light_array
		);
	};
}
