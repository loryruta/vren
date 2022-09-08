#pragma once

#include "vk_helpers/misc.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/debug_utils.hpp"
#include "texture_manager.hpp"

namespace vren
{
	// Forward decl
	class context;

	struct material
	{
		uint32_t m_base_color_texture_idx;
		uint32_t m_metallic_roughness_texture_idx;
		float m_metallic_factor;
		float m_roughness_factor;
		glm::vec4 m_base_color_factor;
	};

	inline uint32_t k_initial_material_count = 1;

	class material_buffer
	{
	private:
		vren::context const* m_context;

	public:
		vren::vk_utils::buffer m_buffer;
		uint32_t m_material_count = 0;

		inline material_buffer(vren::context const& context) :
			m_context(&context),
			m_buffer(vren::vk_utils::alloc_host_visible_buffer(
				*m_context,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VREN_MAX_MATERIAL_COUNT * sizeof(vren::material),
				true
			))
		{
			vren::vk_utils::set_name(context, m_buffer, "material_buffer");

			add_initial_materials();
		}

	private:
		inline void add_initial_materials()
		{
			vren::material* materials = m_buffer.get_mapped_pointer<vren::material>();

			materials[0] = {
				// Default material
				vren::material{
					.m_base_color_texture_idx = vren::texture_manager::k_white_texture,
					.m_metallic_roughness_texture_idx = vren::texture_manager::k_white_texture,
				}
			};
			m_material_count = k_initial_material_count;
		}

	public:
		inline void write_descriptor_set(VkDescriptorSet descriptor_set) const
		{
			vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 0, m_buffer.m_buffer.m_handle, m_material_count * sizeof(vren::material), 0);
		}
	};
}
