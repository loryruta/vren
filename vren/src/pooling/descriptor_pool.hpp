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

	class descriptor_pool : public vren::object_pool<VkDescriptorSet>
	{
	protected:
		explicit descriptor_pool(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::vk_descriptor_set_layout> const& desc_set_layout
		);

		virtual VkDescriptorPool create_descriptor_pool(int max_sets) = 0;
		virtual VkDescriptorSet create_object();

	public:
		std::shared_ptr<vren::context> m_context;
		std::shared_ptr<vren::vk_descriptor_set_layout> m_descriptor_set_layout;

		std::vector<VkDescriptorPool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		~descriptor_pool();
	};

	using pooled_vk_descriptor_set = vren::pooled_object<VkDescriptorSet>;
}
