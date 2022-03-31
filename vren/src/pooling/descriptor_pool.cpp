#include "descriptor_pool.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// ------------------------------------------------------------------------------------------------
// descriptor_pool
// ------------------------------------------------------------------------------------------------

vren::descriptor_pool::descriptor_pool(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{}

vren::vk_descriptor_pool vren::descriptor_pool::create_descriptor_pool(uint32_t max_sets)
{
	// Currently every descriptor set can use at most 32 descriptors per resource type.

	VkDescriptorPoolSize pool_sizes[]{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 32 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 32 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 32 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 32 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 32 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32 }
	};

	VkDescriptorPoolCreateInfo desc_pool_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = max_sets,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	VkDescriptorPool desc_pool;
	vren::vk_utils::check(vkCreateDescriptorPool(m_context->m_device, &desc_pool_info, nullptr, &desc_pool));
	return vren::vk_descriptor_pool(m_context, desc_pool);
}


vren::pooled_vk_descriptor_set vren::descriptor_pool::acquire(VkDescriptorSetLayout desc_set_layout)
{
	auto pooled = try_acquire();
	if (pooled.has_value())
	{
		return std::move(pooled.value());
	}

	if (m_descriptor_pools.empty() || m_last_pool_allocated_count >= VREN_DESCRIPTOR_POOL_SIZE)
	{
		m_descriptor_pools.push_back(create_descriptor_pool(VREN_DESCRIPTOR_POOL_SIZE));
		m_last_pool_allocated_count = 0;
	}

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.descriptorPool = m_descriptor_pools.back().m_handle;
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &desc_set_layout;

	VkDescriptorSet desc_set;
	vren::vk_utils::check(vkAllocateDescriptorSets(m_context->m_device, &alloc_info, &desc_set));

	m_last_pool_allocated_count++;

	return create_managed_object(std::move(desc_set));
}
