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
		vren::rc<vren::texture> m_base_color_texture;
		vren::rc<vren::texture> m_metallic_roughness_texture;

		glm::vec4 m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;

		explicit material(vren::renderer& renderer);
	};

	namespace material_manager
	{
		void update_material_descriptor_set(vren::renderer const& renderer, vren::material const& material, VkDescriptorSet descriptor_set);
	}
}
