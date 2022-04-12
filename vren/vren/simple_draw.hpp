#pragma once

#include <memory>
#include <vector>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "base/resource_container.hpp"
#include "render_list.hpp"
#include "light_array.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
	// Forward decl
	class renderer;
	struct camera;

	// ------------------------------------------------------------------------------------------------
	// Simple draw pass
	// ------------------------------------------------------------------------------------------------

	class simple_draw_pass
	{
	public:
		static constexpr uint32_t k_material_descriptor_set_idx = 0;
		static constexpr uint32_t k_light_array_descriptor_set_idx = 1;
		static constexpr uint32_t k_position_buffer_descriptor_set_idx = 2;

	private:
		vren::context const* m_context;
		vren::renderer* m_renderer;

		vren::vk_utils::pipeline m_pipeline;

		vren::vk_utils::pipeline create_graphics_pipeline();

	public:
		simple_draw_pass(
			vren::context const& ctx,
			vren::renderer& renderer
		);

		void record_commands(
            int frame_idx,
            VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			VkBuffer vertex_buffer,
			VkBuffer index_buffer,
			VkBuffer meshlet_buffer,
			size_t meshlet_count,
			vren::light_array const& lights_array,
			vren::camera const& camera
		);
	};
}

