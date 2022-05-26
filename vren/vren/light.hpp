#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "pool/descriptor_pool.hpp"
#include "vk_helpers/buffer.hpp"
#include "gpu_repr.hpp"
#include "pipeline/render_graph.hpp"

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

		// Dirty flags
		uint32_t m_point_light_buffers_dirty_flags = 0;
		uint32_t m_directional_light_buffers_dirty_flags = 0;
		uint32_t m_spot_light_buffers_dirty_flags = 0;

		// Staging buffers
		vren::vk_utils::buffer m_point_light_staging_buffer;
		vren::vk_utils::buffer m_directional_light_staging_buffer;
		vren::vk_utils::buffer m_spot_light_staging_buffer;

		// GPU buffers
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_point_light_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_directional_light_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_spot_light_buffers;

		vren::vk_descriptor_set_layout m_descriptor_set_layout;
		vren::vk_descriptor_pool m_descriptor_pool;
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_descriptor_sets;

	public:
		// Persistent maps
		point_light* m_point_lights;
		size_t m_point_light_count = 0;

		directional_light* m_directional_lights;
		size_t m_directional_light_count = 0;

		spot_light* m_spot_lights;
		size_t m_spot_light_count = 0;

	public:
		explicit light_array(vren::context const& context);

	private:
		vren::vk_utils::buffer create_staging_buffer(size_t buffer_size, std::string const& buffer_name);
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> create_buffers(size_t buffer_size, std::string const& base_buffer_name);

		vren::vk_descriptor_set_layout create_descriptor_set_layout();

		vren::vk_descriptor_pool create_descriptor_pool();
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> allocate_descriptor_sets();

		void write_descriptor_set(uint32_t frame_idx);

	public:
		void request_point_light_buffer_sync();
		void request_directional_light_buffer_sync();
		void request_spot_light_buffer_sync();

	private:
		vren::render_graph::graph_t create_sync_light_buffer_node(
			vren::render_graph::allocator& allocator,
			char const* node_name,
			vren::vk_utils::buffer const& light_staging_buffer,
			vren::vk_utils::buffer const& light_gpu_buffer,
			size_t single_light_size,
			uint32_t light_count
		);

	public:
		vren::render_graph::graph_t sync_buffers(
			vren::render_graph::allocator& allocator,
			uint32_t frame_idx
		);

		VkDescriptorSet get_descriptor_set(uint32_t frame_idx) const;
	};
}
