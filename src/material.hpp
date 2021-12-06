#pragma once

#include <queue>

#include <glm/glm.hpp>

#include "vk_utils.hpp"
#include "gpu_allocator.hpp"

#define VREN_MATERIAL_TEXTURES_BINDING 1
#define VREN_MATERIAL_COLORS_BINDING   2


#define VREN_DESCRIPTOR_POOL_SIZE 32

namespace vren
{
	// Forward decl
	class renderer;

	struct material_uniform_buffer
	{
		glm::vec3 m_diffuse_color;
		glm::vec3 m_specular_color;
	};

	struct material
	{
		glm::vec3 m_diffuse_color;
		glm::vec3 m_specular_color;

		vren::texture m_diffuse_texture;
		vren::texture m_specular_texture;

		material(vren::renderer& renderer);
	};

	class material_descriptor_set_pool
	{
	private:
		VkDescriptorSetLayout _create_descriptor_set_layout() const;
		void _add_descriptor_pool();

	public:
		VkDescriptorSetLayout m_descriptor_set_layout;

		std::vector<VkDescriptorPool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		std::queue<VkDescriptorSet> m_descriptor_sets;

		vren::renderer& m_renderer;

		material_descriptor_set_pool(vren::renderer& renderer);
		~material_descriptor_set_pool();

		void acquire_descriptor_sets(size_t descriptor_sets_count, VkDescriptorSet* descriptor_sets);
		void update_descriptor_set(vren::material const* material, VkDescriptorSet descriptor_set);
		void release_descriptor_set(VkDescriptorSet descriptor_set);
	};
}
