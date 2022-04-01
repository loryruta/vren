#pragma once

#include <memory>

#include "context.hpp"
#include "renderer.hpp"
#include "resource_container.hpp"
#include "utils/vk_raii.hpp"
#include "utils/buffer.hpp"
#include "utils/shader.hpp"

namespace vren_demo
{
	// ------------------------------------------------------------------------------------------------
	// light_array_movement_buf
	// ------------------------------------------------------------------------------------------------

	class light_array_movement_buf
	{
	private:
		std::shared_ptr<vren::context> m_context;

	public:
		vren::vk_utils::buffer m_point_lights_dirs;

		light_array_movement_buf(
			vren::vk_utils::toolbox const& tb
		);

		void write_descriptor_set(VkDescriptorSet desc_set);
	};

	// ------------------------------------------------------------------------------------------------
	// move_lights
	// ------------------------------------------------------------------------------------------------

	class move_lights : public std::enable_shared_from_this<move_lights>
	{
	public:
		static constexpr uint32_t k_light_arr_desc_set         = 0;
		static constexpr uint32_t k_light_arr_mov_buf_desc_set = 1;

		struct push_constants
		{
			glm::vec3 m_scene_min; float _pad;
			glm::vec3 m_scene_max; float _pad1;
			float m_speed;
		};

	private:
		vren::renderer* m_renderer;

		/* light_array_movement_buf */
		light_array_movement_buf m_light_array_movement_buf;

		std::shared_ptr<vren::vk_utils::self_described_shader> m_shader;
		vren::vk_utils::self_described_compute_pipeline m_pipeline;

		move_lights(vren::renderer& renderer);

	public:
		void dispatch(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			push_constants push_const
		);

		static std::shared_ptr<move_lights> create(vren::renderer& renderer)
		{
			return std::shared_ptr<move_lights>(new move_lights(renderer));
		}
	};
}

