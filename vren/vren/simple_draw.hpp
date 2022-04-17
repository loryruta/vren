#pragma once

#include <memory>
#include <vector>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "base/resource_container.hpp"
#include "light.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
	// Forward decl
	class renderer;
	struct camera;
	struct draw_buffer;

	// ------------------------------------------------------------------------------------------------
	// Simple draw pass
	// ------------------------------------------------------------------------------------------------

	class simple_draw_pass
	{
	public:
		static constexpr uint32_t k_material_descriptor_set_idx = 0;
		static constexpr uint32_t k_light_array_descriptor_set_idx = 1;
		static constexpr uint32_t k_draw_buffer_descriptor_set_idx = 2;

	private:
		vren::context const* m_context;
		VkRenderPass m_render_pass;
		uint32_t m_subpass_idx;

		vren::vk_utils::pipeline m_pipeline;

		vren::vk_utils::pipeline create_graphics_pipeline();

	public:
		simple_draw_pass(vren::context const& context, VkRenderPass render_pass, uint32_t subpass_idx);

		void record_commands(
            int frame_idx,
            VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			vren::draw_buffer const& draw_buf,
			vren::camera const& camera
		);
	};
}

