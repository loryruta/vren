#pragma once

#include <memory>
#include <span>
#include <vector>

#include <volk.h>

#include "object_pool.hpp"
#include "vk_helpers/vk_raii.hpp"

namespace vren
{
    // Forward decl
    class context;

    // ------------------------------------------------------------------------------------------------
    // Descriptor pool
    // ------------------------------------------------------------------------------------------------

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
        vren::context const* m_context;

        uint32_t m_max_sets;
        std::vector<VkDescriptorPoolSize> m_pool_sizes;

        virtual vren::vk_descriptor_pool create_descriptor_pool();
        virtual VkDescriptorSet
        allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout);

    public:
        std::vector<vren::vk_descriptor_pool> m_descriptor_pools;
        size_t m_last_pool_allocated_count = 0;

        explicit descriptor_pool(
            vren::context const& context, uint32_t max_sets, std::span<VkDescriptorPoolSize> const& pool_sizes
        );

        pooled_vk_descriptor_set acquire(VkDescriptorSetLayout desc_set_layout);
    };
} // namespace vren
