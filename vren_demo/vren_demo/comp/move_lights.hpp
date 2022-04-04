#pragma once

#include <memory>

#include <vren/context.hpp>
#include <vren/renderer.hpp>
#include <vren/base/resource_container.hpp>
#include <vren/vk_helpers/vk_raii.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/shader.hpp>

namespace vren_demo
{
	// ------------------------------------------------------------------------------------------------
	// Light array movement buffer
	// ------------------------------------------------------------------------------------------------

	class light_array_movement_buf
	{
	private:
		vren::context const* m_context;

	public:
		vren::vk_utils::buffer m_point_lights_dirs;

		light_array_movement_buf(vren::context const& ctx);

		void write_descriptor_set(VkDescriptorSet desc_set);
	};

	// ------------------------------------------------------------------------------------------------
	// Move lights
	// ------------------------------------------------------------------------------------------------

	class move_lights
	{
	public:
		static constexpr uint32_t k_light_array_descriptor_set_idx = 0;
		static constexpr uint32_t k_light_array_movement_buffer_descriptor_set_idx = 1;

		struct push_constants
		{
			glm::vec3 m_scene_min; float _pad;
			glm::vec3 m_scene_max; float _pad1;
			float m_speed;
			float m_dt;            float _pad2[2];
		};

	private:
		vren::context const* m_context;
		vren::renderer* m_renderer;

		/* light_array_movement_buf */
		light_array_movement_buf m_light_array_movement_buf;

		vren::vk_utils::pipeline m_pipeline;

	public:
		move_lights(vren::context const& ctx, vren::renderer& renderer);

		void dispatch(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			push_constants push_const
		);
	};
}

