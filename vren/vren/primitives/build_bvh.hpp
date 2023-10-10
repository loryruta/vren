#pragma once

#include "vk_api/buffer/buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    struct bvh_node
    {
        inline static const uint32_t k_leaf_node = 0xFFFFFFFFu;
        inline static const uint32_t k_invalid_node = 0xFFFFFFFEu;

        glm::vec3 m_min;
        uint32_t m_next;
        glm::vec3 m_max;
        uint32_t _pad;

        inline bool is_leaf() const { return m_next == k_leaf_node; };
        inline bool is_invalid() const { return m_next == k_invalid_node; };
    };

    class build_bvh
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;

    private:
        vren::context const* m_context;

        vren::pipeline m_pipeline;

    public:
        build_bvh(vren::context const& context);

    private:
        void
        write_descriptor_set(VkDescriptorSet descriptor_set, vren::vk_utils::buffer const& buffer, uint32_t leaf_count);

    public:
        static VkBufferUsageFlags get_required_buffer_usage_flags();
        static size_t get_required_buffer_size(uint32_t leaf_count);

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer, // Its size must be at least ::get_buffer_length(leaf_count)
            uint32_t leaf_count
        );
    };

    uint32_t calc_bvh_padded_leaf_count(uint32_t leaf_count);
    uint32_t calc_bvh_buffer_length(uint32_t leaf_count);
    size_t calc_bvh_buffer_size(uint32_t leaf_count);
    uint32_t calc_bvh_root_index(uint32_t leaf_count);
    uint32_t calc_bvh_level_count(uint32_t leaf_count);
} // namespace vren
