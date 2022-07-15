#pragma once

#include "vk_helpers/shader.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/vk_raii.hpp"

namespace vren
{
    class bucket_sort
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;
        inline static const uint32_t k_max_items = 1;

        inline static const uint32_t k_key_size = sizeof(uint16_t);

    private:
        vren::context const* m_context;

        vren::vk_descriptor_set_layout m_descriptor_set_layout;

        vren::pipeline m_count_pipeline;
        vren::pipeline m_write_pipeline;

    public:
        bucket_sort(vren::context const& context);

    private:
        vren::vk_descriptor_set_layout create_descriptor_set_layout();

        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& input_buffer,
            uint32_t length,
            vren::vk_utils::buffer const& scratch_buffer_1
        );

    public:
        vren::vk_utils::buffer create_scratch_buffer_1(uint32_t length);

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            vren::vk_utils::buffer const& scratch_buffer_1
        );
    };
}

