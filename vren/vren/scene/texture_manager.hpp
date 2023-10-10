#pragma once

#include <vector>

#include "pool/DescriptorPool.hpp"
#include "vk_api/image/utils.hpp"

namespace vren
{
    // Forward decl
    class context;
    class toolbox;

    // ------------------------------------------------------------------------------------------------
    // texture_manager_descriptor_pool
    // ------------------------------------------------------------------------------------------------

    class texture_manager_descriptor_pool : public vren::DescriptorPool
    {
    protected:
        VkDescriptorSet
        allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout) override;

    public:
        explicit texture_manager_descriptor_pool(
            vren::context const& context, uint32_t max_sets, std::span<VkDescriptorPoolSize> const& pool_sizes
        );
        ~texture_manager_descriptor_pool() = default;
    };

    // ------------------------------------------------------------------------------------------------
    // texture_manager
    // ------------------------------------------------------------------------------------------------

    class texture_manager
    {
        friend toolbox;

        vren::context const* m_context;
        vren::vk_descriptor_set_layout m_descriptor_set_layout;
        vren::texture_manager_descriptor_pool m_descriptor_pool;

    public:
        static constexpr uint32_t k_max_texture_count = 65536;

        std::vector<vren::vk_utils::texture> m_textures;                  // todo private
        std::shared_ptr<vren::pooled_vk_descriptor_set> m_descriptor_set; // todo private

        explicit texture_manager(vren::context const& context);
        ~texture_manager() = default;

        void rewrite_descriptor_set();

    private:
        vren::vk_descriptor_set_layout create_descriptor_set_layout();
        vren::texture_manager_descriptor_pool create_descriptor_pool();
    };
} // namespace vren
