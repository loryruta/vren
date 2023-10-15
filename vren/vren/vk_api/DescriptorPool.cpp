#include "DescriptorPool.hpp"

#include "Context.hpp"
#include "misc_utils.hpp"

using namespace vren;

// ------------------------------------------------------------------------------------------------ PooledDescriptorSet

PooledDescriptorSet::PooledDescriptorSet(VkDescriptorSet handle, VkDescriptorPool descriptor_pool) :
    m_handle(handle),
    m_descriptor_pool(descriptor_pool)
{
    assert(handle);
    assert(descriptor_pool);
}

PooledDescriptorSet::PooledDescriptorSet(PooledDescriptorSet&& other) noexcept :
    m_handle(other.m_handle),
    m_descriptor_pool(other.m_descriptor_pool)
{
    other.m_handle = VK_NULL_HANDLE;
}

PooledDescriptorSet::~PooledDescriptorSet()
{
    if (m_handle != VK_NULL_HANDLE)
        vkFreeDescriptorSets(Context::get().device().handle(), m_descriptor_pool, 1, &m_handle);
}

// ------------------------------------------------------------------------------------------------ DescriptorSetPool

DescriptorPool::DescriptorPool(uint32_t max_sets, std::span<VkDescriptorPoolSize> pool_sizes) :
    m_max_sets(max_sets),
    m_pool_sizes(pool_sizes.begin(), pool_sizes.end())
{
}

VkDescriptorPool DescriptorPool::create_descriptor_pool()
{
    VkDescriptorPoolCreateInfo descriptor_pool_info{};
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_info.maxSets = m_max_sets;
    descriptor_pool_info.poolSizeCount = (uint32_t) m_pool_sizes.size();
    descriptor_pool_info.pPoolSizes = m_pool_sizes.data();

    VkDescriptorPool descriptor_pool;
    VREN_CHECK(vkCreateDescriptorPool(Context::get().device().handle(), &descriptor_pool_info, nullptr, &descriptor_pool));
    return descriptor_pool;
}

VkDescriptorSet DescriptorPool::allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, void const* next_ptr)
{
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.pNext = next_ptr;
    descriptor_set_alloc_info.descriptorPool = descriptor_pool;
    descriptor_set_alloc_info.descriptorSetCount = 1;
    descriptor_set_alloc_info.pSetLayouts = &descriptor_set_layout;

    VkDescriptorSet descriptor_set;
    VkResult result = vkAllocateDescriptorSets(Context::get().device().handle(), &descriptor_set_alloc_info, &descriptor_set);
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY)
        return VK_NULL_HANDLE;
    VREN_CHECK(result);
    return descriptor_set;
}

PooledDescriptorSet&& DescriptorPool::acquire(VkDescriptorSetLayout descriptor_set_layout, void const* next_ptr)
{
    VkDescriptorPool owner_pool = VK_NULL_HANDLE;
    VkDescriptorSet allocated_set = VK_NULL_HANDLE;

    for (int i = 0; i < m_descriptor_pools.size(); i++)
    {
        VkDescriptorPool descriptor_pool = m_descriptor_pools.at(i).get();
        allocated_set = allocate_descriptor_set(descriptor_pool, descriptor_set_layout, next_ptr);
        if (allocated_set != VK_NULL_HANDLE)
        {
            owner_pool = descriptor_pool;
            break;
        }
    }

    if (allocated_set == VK_NULL_HANDLE) // No set was allocated, we need to instantiate another pool
    {
        owner_pool = create_descriptor_pool();
        m_descriptor_pools.emplace_back(owner_pool);

        allocated_set = allocate_descriptor_set(owner_pool, descriptor_set_layout, next_ptr);
    }

    return PooledDescriptorSet(allocated_set, owner_pool);
}

DescriptorPool& DescriptorPool::get_default()
{
    if (!s_default_instance)
    {
        uint32_t max_sets = 32;
        std::vector<VkDescriptorPoolSize> pool_sizes = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 32},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 32},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 32},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 32},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 32},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32}};
        s_default_instance = std::make_unique<DescriptorPool>(max_sets, pool_sizes);
    }
    return *s_default_instance;
}
