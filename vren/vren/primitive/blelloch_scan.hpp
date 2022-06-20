#pragma once

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    class blelloch_scan
    {
    private:
        vren::context const* m_context;

        vren::pipeline m_upsweep_pipeline;
        vren::pipeline m_downsweep_pipeline;

    public:
        blelloch_scan(vren::context const& context);

    private:
        void write_descriptor_set(VkDescriptorSet descriptor_set, vren::vk_utils::buffer const& buffer);

    public:
        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t stride = 1,
            uint32_t offset = 0
        );
    };
}
