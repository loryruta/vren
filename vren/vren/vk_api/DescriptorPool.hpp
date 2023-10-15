#pragma once

#include <memory>
#include <span>
#include <vector>

#include <volk.h>

#include "vk_raii.hpp"

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

        VkDescriptorSet set() const { return m_handle; }
        VkDescriptorPool pool() const { return m_descriptor_pool; }
    };

    // ------------------------------------------------------------------------------------------------ DescriptorSetPool

    /// An unlimited pool of VkDescriptorSet, of any given VkDescriptorSetLayout.
    class DescriptorPool
    {
    private:
        static std::unique_ptr<DescriptorPool> s_default_instance;

        uint32_t m_max_sets;
        std::vector<VkDescriptorPoolSize> m_pool_sizes;

        std::vector<vk_descriptor_pool> m_descriptor_pools{};

    public:
        explicit DescriptorPool(uint32_t max_sets, std::span<VkDescriptorPoolSize> pool_sizes);
        ~DescriptorPool() = default;

        /// \param next_ptr Additional struct chain added to VkDescriptorSetAllocateInfo::pNext
        PooledDescriptorSet&& acquire(VkDescriptorSetLayout descriptor_set_layout, void const* next_ptr = nullptr);

        static DescriptorPool& get_default();

    private:
        VkDescriptorPool create_descriptor_pool();
        VkDescriptorSet allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout, void const* next_ptr);
    };
} // namespace vren
