#pragma once

#include "vk_helpers/shader.hpp"
#include "vk_helpers/buffer.hpp"
#include "primitives/blelloch_scan.hpp"

namespace vren
{
    class radix_sort
    {
    public:
        inline static constexpr uint32_t k_radix_bits = 4;
        inline static constexpr uint32_t k_radix = 1 << k_radix_bits;

    private:
        vren::context const* m_context;

        vren::vk_descriptor_set_layout m_descriptor_set_layout;

        vren::pipeline m_local_count_pipeline;
        vren::pipeline m_global_offset_pipeline;
        vren::pipeline m_reorder_pipeline;

    public:
        radix_sort(vren::context const& context);

    private:
        vren::vk_descriptor_set_layout create_descriptor_set_layout();

        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& input_buffer,
            vren::vk_utils::buffer const& output_buffer,
            uint32_t length,
            vren::vk_utils::buffer const& scratch_buffer_1
        );

    public:
        vren::vk_utils::buffer create_scratch_buffer_1(uint32_t length);
        vren::vk_utils::buffer create_scratch_buffer_2(uint32_t length);

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            vren::vk_utils::buffer const& scratch_buffer_1,
            vren::vk_utils::buffer const& scratch_buffer_2
         );
    };
}
