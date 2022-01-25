#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "vk_utils.hpp"
#include "gpu_allocator.hpp"
#include "light.hpp"

namespace vren
{
	// Forward decl
	class renderer;

	struct material
	{
		std::shared_ptr<vren::texture> m_base_color_texture;
		std::shared_ptr<vren::texture> m_metallic_roughness_texture; // b = metallic / g = roughness

		glm::vec4 m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;

		explicit material(std::shared_ptr<vren::renderer> const& renderer);
	};

	namespace material_manager
	{
		void update_material_descriptor_set(vren::renderer const& renderer, vren::material const& material, VkDescriptorSet descriptor_set);
	}
}
