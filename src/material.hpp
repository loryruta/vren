#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "context.hpp"
#include "vk_utils.hpp"
#include "gpu_allocator.hpp"
#include "light.hpp"

namespace vren
{
	struct material
	{
		std::shared_ptr<vren::texture> m_base_color_texture;
		std::shared_ptr<vren::texture> m_metallic_roughness_texture; // b = metallic / g = roughness

		glm::vec4 m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;

		explicit material(std::shared_ptr<vren::context> const& ctx);
	};

	namespace material_manager
	{
		void update_material_descriptor_set(vren::context const& ctx, vren::material const& material, VkDescriptorSet descriptor_set);
	}
}
