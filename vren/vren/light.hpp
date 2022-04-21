#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "pools/descriptor_pool.hpp"
#include "vk_helpers/buffer.hpp"
#include "gpu_repr.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Light array
	// ------------------------------------------------------------------------------------------------

	class light_array
	{
	public:
		static constexpr size_t k_max_point_light_count = 32768;
		static constexpr size_t k_max_directional_light_count = 32768;
		static constexpr size_t k_max_spot_light_count = 32768;

		static constexpr size_t k_point_light_buffer_size = k_max_point_light_count * sizeof(point_light);
		static constexpr size_t k_directional_light_buffer_size = k_max_directional_light_count * sizeof(directional_light);
		static constexpr size_t k_spot_light_buffer_size = k_max_spot_light_count * sizeof(spot_light);

	private:
		vren::context const* m_context;

		uint32_t m_point_light_buffers_dirty_flags = 0;
		uint32_t m_directional_light_buffers_dirty_flags = 0;
		uint32_t m_spot_light_buffers_dirty_flags = 0;

		vren::vk_descriptor_set_layout m_descriptor_set_layout;
		vren::vk_descriptor_pool m_descriptor_pool;

		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_point_light_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_directional_light_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_spot_light_buffers;

		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_descriptor_sets;

		vren::vk_descriptor_set_layout create_descriptor_set_layout();
		vren::vk_descriptor_pool create_descriptor_pool();
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> create_buffers(size_t buffer_size);
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> allocate_descriptor_sets();
		void write_descriptor_set(uint32_t frame_idx);

	public:
		std::array<point_light, k_max_point_light_count> m_point_lights;
		size_t m_point_light_count = 0;

		std::array<directional_light, k_max_directional_light_count> m_directional_lights;
		size_t m_directional_light_count = 0;

		std::array<spot_light, k_max_spot_light_count> m_spot_lights;
		size_t m_spot_light_count = 0;

		explicit light_array(vren::context const& context);

		void request_point_light_buffer_sync();
		void request_directional_light_buffer_sync();
		void request_spot_light_buffer_sync();

		void sync_buffers(uint32_t frame_idx);

		VkDescriptorSet get_descriptor_set(uint32_t frame_idx) const;
	};
}
