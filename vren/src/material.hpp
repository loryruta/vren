#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "pooling/descriptor_pool.hpp"
#include "utils/image.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// material
	// ------------------------------------------------------------------------------------------------

	struct material
	{
		std::shared_ptr<vren::vk_utils::texture> m_base_color_texture;
		std::shared_ptr<vren::vk_utils::texture> m_metallic_roughness_texture; // b = metallic / g = roughness

		glm::vec4 m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;

		explicit material(vren::vk_utils::toolbox const& tb);
	};

	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_descriptor_set_layout create_material_descriptor_set_layout(std::shared_ptr<vren::context> const& ctx);
	void update_material_descriptor_set(vren::context const& ctx, vren::material const& mat, VkDescriptorSet desc_set);
}
