#pragma once

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"
#include "vk_helpers/vk_raii.hpp"

namespace vren
{
    class bucket_sort
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;
        inline static const uint32_t k_max_items = 1;

        inline static const uint32_t k_key_size = 1 << 16;
        inline static const uint32_t k_key_mask = k_key_size - 1;

    private:
        vren::context const* m_context;

        vren::vk_descriptor_set_layout m_descriptor_set_layout;

        vren::pipeline m_count_pipeline;
        vren::pipeline m_write_pipeline;

    public:
        bucket_sort(vren::context const& context);

    private:
        vren::vk_descriptor_set_layout create_descriptor_set_layout();

    public:
        static VkBufferUsageFlags get_required_output_buffer_usage_flags();
        static size_t get_required_output_buffer_size(uint32_t length);

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& input_buffer,
            uint32_t input_buffer_length,
            size_t input_buffer_offset,
            vren::vk_utils::buffer const& output_buffer,
            size_t output_buffer_offset
        );
    };
} // namespace vren
