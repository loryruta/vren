#pragma once

#include <functional>

#include <glm/glm.hpp>

#include "base/resource_container.hpp"
#include "vk_helpers/buffer.hpp"
#include "gpu_repr.hpp"
#include "pipeline/render_graph.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Material manager
	// ------------------------------------------------------------------------------------------------

	class material_manager
	{
	public:
		friend vren::toolbox;

	public:
		static constexpr size_t k_max_material_count = 16384;
		static constexpr size_t k_max_material_buffer_size = k_max_material_count * sizeof(vren::material); // ~656 KB

	private:
		vren::context const* m_context;

		uint32_t m_buffers_dirty_flags = 0;

		vren::vk_descriptor_set_layout m_descriptor_set_layout;
		vren::vk_descriptor_pool m_descriptor_pool;
		vren::vk_utils::buffer m_staging_buffer;

	public:
		vren::material* m_materials;
		size_t m_material_count = 0;

	private:
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_buffers;
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_descriptor_sets;

	public:
		explicit material_manager(vren::context const& context);

	private:
		vren::vk_utils::buffer create_staging_buffer();
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> create_buffers();

		vren::vk_descriptor_set_layout create_descriptor_set_layout();

		vren::vk_descriptor_pool create_descriptor_pool();
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> allocate_descriptor_sets();

		void write_descriptor_set(uint32_t frame_idx);

		void lazy_initialize();

	public:
		void request_buffer_sync();
		vren::render_graph::graph_t sync_buffer(vren::render_graph::allocator& allocator, uint32_t frame_idx);

		VkDescriptorSet get_descriptor_set(uint32_t frame_idx) const;
		VkBuffer get_buffer(uint32_t frame_idx) const;
	};
}
