#pragma once

#include <memory>
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
		void create_material_layout();
		void create_lights_array_layout();

		void decorate_material_descriptor_pool_sizes(std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes);
		void decorate_lights_array_descriptor_pool_sizes(std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes);

		void add_descriptor_pool();

	public:
		std::shared_ptr<vren::renderer> m_renderer;

		VkDescriptorSetLayout m_material_layout;
		VkDescriptorSetLayout m_lights_array_layout;

		std::vector<VkDescriptorPool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		std::queue<VkDescriptorSet> m_descriptor_sets;

		explicit descriptor_set_pool(std::shared_ptr<vren::renderer> const& renderer);
		~descriptor_set_pool();

		void acquire_descriptor_sets(size_t descriptor_sets_count, VkDescriptorSetLayout* descriptor_set_layouts, VkDescriptorSet* descriptor_sets);

		void acquire_material_descriptor_set(VkDescriptorSet* descriptor_set);
		void acquire_lights_array_descriptor_set(VkDescriptorSet* descriptor_set);

		void release_descriptor_set(VkDescriptorSet descriptor_set);
	};
}
