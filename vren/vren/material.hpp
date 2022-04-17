#pragma once

#include <functional>

#include <glm/glm.hpp>

#include "vk_helpers/buffer.hpp"
#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Material
	// ------------------------------------------------------------------------------------------------

	struct material
	{
		uint32_t m_base_color_texture_idx;         float _pad[3];
		uint32_t m_metallic_roughness_texture_idx; float _pad1[3];
		glm::vec3 m_base_color_factor; float _pad2;
		float m_metallic_factor;       float _pad3[3];
		float m_roughness_factor;      float _pad4[3];
	};

	// ------------------------------------------------------------------------------------------------
	// Material manager
	// ------------------------------------------------------------------------------------------------

	class material_manager
	{
	public:
		static constexpr size_t k_max_material_count = 8192;
		static constexpr size_t k_max_material_buffer_size = k_max_material_count * sizeof(material); // ~656 KB

	private:
		vren::context const* m_context;

		uint32_t m_buffers_dirty_flags = 0;

		vren::vk_descriptor_set_layout m_descriptor_set_layout;
		vren::vk_descriptor_pool m_descriptor_pool;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_buffers;
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_descriptor_sets;

	public:
		std::array<vren::material, k_max_material_count> m_materials;
		size_t m_material_count = 0;

		explicit material_manager(vren::context const& context);

	private:
		vren::vk_descriptor_set_layout create_descriptor_set_layout();
		vren::vk_descriptor_pool create_descriptor_pool();
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> create_buffers();
		std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> allocate_descriptor_sets();

		void write_descriptor_set(uint32_t frame_idx);

	public:
		void request_buffer_sync();
		void sync_buffer(uint32_t frame_idx);

		VkDescriptorSet get_descriptor_set(uint32_t frame_idx) const;
	};
}
