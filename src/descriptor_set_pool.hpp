#pragma once

#include <vector>
#include <queue>

#include <vulkan/vulkan.h>

namespace vren
{
	// Forward decl
	class renderer;

	class descriptor_set_pool
	{
	private:
		void _create_material_layout();
		void _create_lights_array_layout();

		void _decorate_material_descriptor_pool_sizes(std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes);
		void _decorate_lights_array_descriptor_pool_sizes(std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes);

		void _add_descriptor_pool();

	public:
		VkDescriptorSetLayout m_material_layout;
		VkDescriptorSetLayout m_lights_array_layout;

		std::vector<VkDescriptorPool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		std::queue<VkDescriptorSet> m_descriptor_sets;

		vren::renderer& m_renderer;

		explicit descriptor_set_pool(vren::renderer& renderer);
		~descriptor_set_pool();

		void _acquire_descriptor_sets(size_t descriptor_sets_count, VkDescriptorSetLayout* descriptor_set_layouts, VkDescriptorSet* descriptor_sets);

		void acquire_material_descriptor_set(VkDescriptorSet* descriptor_set);
		void acquire_lights_array_descriptor_set(VkDescriptorSet* descriptor_set);

		void release_descriptor_set(VkDescriptorSet descriptor_set);
	};
}
