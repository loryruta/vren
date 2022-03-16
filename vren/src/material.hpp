#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "context.hpp"
#include "pooling/descriptor_pool.hpp"
#include "utils/image.hpp"

namespace vren
{
	// --------------------------------------------------------------------------------------------------------------------------------
	// material
	// --------------------------------------------------------------------------------------------------------------------------------

	struct material
	{
		std::shared_ptr<vren::vk_utils::texture> m_base_color_texture;
		std::shared_ptr<vren::vk_utils::texture> m_metallic_roughness_texture; // b = metallic / g = roughness

		glm::vec4 m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;

		explicit material(std::shared_ptr<vren::context> const& ctx);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// material_descriptor_pool
	// --------------------------------------------------------------------------------------------------------------------------------

	class material_descriptor_pool : public vren::descriptor_pool
	{
	public:
		material_descriptor_pool(std::shared_ptr<vren::context> const& ctx);

		VkDescriptorPool create_descriptor_pool(int max_sets);
	};

	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_descriptor_set_layout create_material_descriptor_set_layout(std::shared_ptr<vren::context> const& ctx);
	void update_material_descriptor_set(vren::context const& ctx, vren::material const& mat, VkDescriptorSet desc_set);
}
