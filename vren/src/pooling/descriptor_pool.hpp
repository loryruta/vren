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

	struct managed_vk_descriptor_set // TODO don't really like this class, untangle descriptor_pool from object_pool<_t>
	{
		VkDescriptorSetLayout m_descriptor_set_layout;
		VkDescriptorPool m_descriptor_pool;
		VkDescriptorSet m_descriptor_set;
	};

	using pooled_vk_descriptor_set = vren::pooled_object<managed_vk_descriptor_set>;

	class descriptor_pool : public vren::object_pool<managed_vk_descriptor_set>
	{
	protected:
		vren::vk_descriptor_pool create_descriptor_pool(uint32_t max_sets);

	public:
		std::shared_ptr<vren::context> m_context;

		std::vector<vren::vk_descriptor_pool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		explicit descriptor_pool(std::shared_ptr<vren::context> const& ctx);

		pooled_vk_descriptor_set acquire(VkDescriptorSetLayout desc_set_layout);

		static std::shared_ptr<descriptor_pool> create(std::shared_ptr<vren::context> const& ctx)
		{
			return std::make_shared<descriptor_pool>(ctx);
		}
	};
}
