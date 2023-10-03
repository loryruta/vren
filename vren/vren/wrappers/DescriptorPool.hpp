#pragma once

#include <memory>
#include <span>
#include <vector>

#include <volk.h>

namespace vren
{
    // Forward decl
    class Context;
    class DescriptorPool;

    // ------------------------------------------------------------------------------------------------ PooledDescriptorSet

    class PooledDescriptorSet
    {
        friend class DescriptorPool;

    private:
        VkDescriptorSet m_handle;
        VkDescriptorPool m_descriptor_pool;

        explicit PooledDescriptorSet(VkDescriptorSet handle, VkDescriptorPool descriptor_pool);

    public:
        PooledDescriptorSet(PooledDescriptorSet const& other) = delete;
        PooledDescriptorSet(PooledDescriptorSet&& other) noexcept;
        ~PooledDescriptorSet();
    };

    // ------------------------------------------------------------------------------------------------ DescriptorSetPool

    /// An unlimited pool of VkDescriptorSet, of any given VkDescriptorSetLayout.
    class DescriptorPool
    {
    public:
        static const int k_max_sets = 32;
        static const std::vector<VkDescriptorPoolSize> k_pool_sizes;

    private:
        std::vector<HandleDeleter<VkDescriptorPool>> m_descriptor_pools{};

    public:
        /// \param next_ptr Additional struct chain added to VkDescriptorSetAllocateInfo::pNext
        PooledDescriptorSet acquire(VkDescriptorSetLayout descriptor_set_layout, void const* next_ptr);

    private:
        VkDescriptorPool create_descriptor_pool();
        VkDescriptorSet allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, void const* next_ptr);
    };
} // namespace vren
