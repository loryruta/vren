#include "descriptor_pool.hpp"

#include <stdexcept>

#include "utils/image.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// descriptor_pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::descriptor_pool::descriptor_pool(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{
	_alloc_descriptor_pool(); // TODO do not allocate as soon as initialized (could be an unused descriptor_pool)
}

vren::descriptor_pool::~descriptor_pool()
{
	for (VkDescriptorPool descriptor_pool : m_descriptor_pools) {
		vkDestroyDescriptorPool(m_context->m_device, descriptor_pool, nullptr);
	}
}

void vren::descriptor_pool::_release_descriptor_set(vren::vk_descriptor_set const& desc_set)
{
	m_pooled_descriptor_sets.push_back(desc_set.m_handle);
}

void vren::descriptor_pool::_alloc_descriptor_pool()
{
	m_descriptor_pools.push_back(
		create_descriptor_pool(VREN_DESCRIPTOR_POOL_SIZE)
	);

	m_last_pool_allocated_count = 0;
}

vren::vk_descriptor_set vren::descriptor_pool::acquire_descriptor_set(VkDescriptorSetLayout desc_set_layout)
{
	if (m_last_pool_allocated_count >= VREN_DESCRIPTOR_POOL_SIZE)
	{
		_alloc_descriptor_pool();
	}

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.descriptorPool = m_descriptor_pools.back();
	alloc_info.descriptorSetCount = 1;
	alloc_info.pSetLayouts = &desc_set_layout;

	VkDescriptorSet desc_set;
	vren::vk_utils::check(vkAllocateDescriptorSets(m_context->m_device, &alloc_info, &desc_set));

	m_last_pool_allocated_count++;

	return vren::vk_descriptor_set(shared_from_this(), desc_set);
}

// --------------------------------------------------------------------------------------------------------------------------------
// vk_descriptor_set
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_descriptor_set::vk_descriptor_set(
	std::shared_ptr<descriptor_pool> const& descriptor_pool,
	VkDescriptorSet handle
) :
	m_descriptor_pool(descriptor_pool),
	m_handle(handle)
{}

vren::vk_descriptor_set::vk_descriptor_set(
	vk_descriptor_set&& other
)
{
	*this = std::move(other);
}

vren::vk_descriptor_set::~vk_descriptor_set()
{
	if (m_handle != VK_NULL_HANDLE) {
		m_descriptor_pool->_release_descriptor_set(*this);
	}
}

vren::vk_descriptor_set& vren::vk_descriptor_set::operator=(vk_descriptor_set&& other)
{
	std::swap(m_descriptor_pool, other.m_descriptor_pool);
	m_handle = other.m_handle;

	other.m_handle = VK_NULL_HANDLE;

	return *this;
}
