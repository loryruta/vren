#pragma once

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"
#include "primitives/reduce.hpp"

namespace vren
{
    class blelloch_scan
    {
    public:
        inline static const uint32_t k_downsweep_subgroup_size = 32;
        inline static const uint32_t k_downsweep_num_iterations = 8;

        inline static const uint32_t k_subgroup_downsweep_subgroup_size = 32;
        inline static const uint32_t k_subgroup_downseep_num_iterations = 16;

        inline static const uint32_t k_min_buffer_length = k_subgroup_downsweep_subgroup_size * k_subgroup_downseep_num_iterations;

    private:
        vren::context const* m_context;
        vren::pipeline m_downsweep_pipeline, m_subgroup_downsweep_pipeline;

    public:
        blelloch_scan(vren::context const& context);

    private:
        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset
        );

    public:
        void downsweep(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset,
            bool clear_last
        );

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset
        );
    };
}
