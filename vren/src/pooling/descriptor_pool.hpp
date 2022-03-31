#pragma once

#include <memory>
#include <vector>
#include <queue>

#include <vulkan/vulkan.h>

#include "object_pool.hpp"
#include "utils/vk_raii.hpp"

namespace vren
{
	// Forward decl
	class context;

	// --------------------------------------------------------------------------------------------------------------------------------
	// descriptor_pool
	// --------------------------------------------------------------------------------------------------------------------------------

	using pooled_vk_descriptor_set = vren::pooled_object<VkDescriptorSet>;

	class descriptor_pool : public vren::object_pool<VkDescriptorSet>
	{
	protected:
		vren::vk_descriptor_pool create_descriptor_pool(uint32_t max_sets);

	public:
		std::shared_ptr<vren::context> m_context;

		std::vector<vren::vk_descriptor_pool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		explicit descriptor_pool(std::shared_ptr<vren::context> const& ctx);

		pooled_vk_descriptor_set acquire(VkDescriptorSetLayout desc_set_layout);
	};
}
