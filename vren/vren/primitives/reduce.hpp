#pragma once

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    class reduce
    {
    public:
        inline static const uint32_t k_subgroup_size  = 32;
        inline static const uint32_t k_num_iterations = 16;
        inline static const uint32_t k_min_buffer_length = k_subgroup_size * k_num_iterations;

    private:
        vren::context const* m_context;
        vren::pipeline m_pipeline;

    public:
        reduce(vren::context const& context);

    private:
        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset
        );

    public:
        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset = 0,
            uint32_t* result = nullptr
        );
    };
}
