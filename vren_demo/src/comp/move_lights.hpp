#pragma once

#include <memory>

#include "context.hpp"
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

		light_array_movement_buf(vren::vk_utils::toolbox const& tb);

		void write_descriptor_set(VkDescriptorSet desc_set);

		static vren::vk_descriptor_set_layout create_descriptor_set_layout(std::shared_ptr<vren::context> const& ctx);
		static vren::vk_descriptor_pool create_descriptor_pool(std::shared_ptr<vren::context> const& ctx, uint32_t max_sets);
	};

	// ------------------------------------------------------------------------------------------------
	// move_lights
	// ------------------------------------------------------------------------------------------------

	class move_lights : public std::enable_shared_from_this<move_lights>
	{
	public:
		struct push_constants
		{
			glm::vec3 m_scene_min; float _pad;
			glm::vec3 m_scene_max; float _pad1;
			float m_speed;
		};

	private:
		/* light_array_movement_buf */
		light_array_movement_buf       m_light_array_movement_buf;

		vren::vk_descriptor_set_layout m_light_array_movement_buf_descriptor_set_layout; // TODO layout and pool could be required anywhere: make them global?
		vren::vk_descriptor_pool       m_light_array_movement_buf_descriptor_pool;
		VkDescriptorSet                m_light_array_movement_buf_descriptor_set;

		/* */
		vren::vk_utils::pipeline m_pipeline;

		move_lights(vren::vk_utils::toolbox const& tb);

	public:
		void dispatch(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			push_constants push_const,
			VkDescriptorSet light_array_buf_desc_set // It's lifetime must be externally ensured (aka. we expect it to already be present in resource container)
		);

		static std::shared_ptr<move_lights> create(vren::vk_utils::toolbox const& tb)
		{
			return std::shared_ptr<move_lights>(new move_lights(tb));
		}
	};
}

