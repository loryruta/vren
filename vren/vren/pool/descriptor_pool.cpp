#include "descriptor_pool.hpp"

#include "context.hpp"
#include "vk_helpers/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Descriptor pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::descriptor_pool::descriptor_pool(
    vren::context const& context, uint32_t max_sets, std::span<VkDescriptorPoolSize> const& pool_sizes
) :
    m_context(&context),
    m_max_sets(max_sets),
    m_pool_sizes(pool_sizes.begin(), pool_sizes.end())
{
}

vren::vk_descriptor_pool vren::descriptor_pool::create_descriptor_pool()
{
    VkDescriptorPoolCreateInfo descriptor_pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = m_max_sets,
        .poolSizeCount = (uint32_t) m_pool_sizes.size(),
        .pPoolSizes = m_pool_sizes.data()};
    VkDescriptorPool descriptor_pool;
    VREN_CHECK(
        vkCreateDescriptorPool(m_context->m_device, &descriptor_pool_info, nullptr, &descriptor_pool), m_context
    );
    return vren::vk_descriptor_pool(*m_context, descriptor_pool);
}

VkDescriptorSet vren::descriptor_pool::allocate_descriptor_set(
    VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout
)
{
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout};
    VkDescriptorSet descriptor_set;
    VREN_CHECK(vkAllocateDescriptorSets(m_context->m_device, &descriptor_set_alloc_info, &descriptor_set), m_context);
    return descriptor_set;
}

vren::pooled_vk_descriptor_set vren::descriptor_pool::acquire(VkDescriptorSetLayout desc_set_layout)
{
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;

    if (m_unused_objects.size() > 0)
    {
        // If an unused descriptor set allocated for the requested descriptor set layout is found, then returns it.

        for (auto it = m_unused_objects.begin(); it != m_unused_objects.end(); it++)
        {
            auto desc_set = *it;

            if (desc_set.m_descriptor_set_layout == desc_set_layout)
            {
                m_unused_objects.erase(it);
                return create_managed_object(std::move(desc_set));
            }
        }

        // Otherwise takes the first descriptor set (which is the less used), frees it from its pool and
        // allocate a new descriptor set from it for the given layout.

        auto desc_set = m_unused_objects.front();
        m_unused_objects.erase(m_unused_objects.begin()); // TODO not efficient, prefer the use of std::deque

        vkFreeDescriptorSets(m_context->m_device, desc_set.m_descriptor_pool, 1, &desc_set.m_descriptor_set);

        desc_pool = desc_set.m_descriptor_pool;
    }
    else if (m_descriptor_pools.empty() || m_last_pool_allocated_count >= m_max_sets)
    {
        // If there's no unused object and the descriptor pool is full, we need to create a new descriptor pool
        // and allocate the new descriptor set from there.

        m_descriptor_pools.push_back(create_descriptor_pool());
        desc_pool = m_descriptor_pools.back().m_handle;

        m_last_pool_allocated_count = 0;
    }
    else
    {
        desc_pool = m_descriptor_pools.back().m_handle;
    }

    m_last_pool_allocated_count++;

    VkDescriptorSet descriptor_set = allocate_descriptor_set(desc_pool, desc_set_layout);

    return create_managed_object({
        .m_descriptor_set_layout = desc_set_layout,
        .m_descriptor_pool = desc_pool,
        .m_descriptor_set = descriptor_set,
    });
}
