#include "descriptor_pool.hpp"

#include <stdexcept>

#include "utils/image.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// descriptor_pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::descriptor_pool::descriptor_pool(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<vren::vk_descriptor_set_layout> const& desc_set_layout
) :
	m_context(ctx),
	m_descriptor_set_layout(desc_set_layout)
{}

vren::descriptor_pool::~descriptor_pool()
{
	for (VkDescriptorPool descriptor_pool : m_descriptor_pools) {
		vkDestroyDescriptorPool(m_context->m_device, descriptor_pool, nullptr);
	}
}

VkDescriptorSet vren::descriptor_pool::create_object()
{
	if (m_descriptor_pools.empty() || m_last_pool_allocated_count >= VREN_DESCRIPTOR_POOL_SIZE)
	{
		m_descriptor_pools.push_back(create_descriptor_pool(VREN_DESCRIPTOR_POOL_SIZE));
		m_last_pool_allocated_count = 0;
	}

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.descriptorPool = m_descriptor_pools.back();
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &m_descriptor_set_layout->m_handle;

	VkDescriptorSet desc_set;
	vren::vk_utils::check(vkAllocateDescriptorSets(m_context->m_device, &alloc_info, &desc_set));

	m_last_pool_allocated_count++;

	return desc_set;
}
