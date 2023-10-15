#pragma once

#include "primitives/Reduce.hpp"
#include "vk_api/buffer/Buffer.hpp"
#include "vk_api/shader/ComputePipeline.hpp"

namespace vren
{
    class BlellochScan
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;
        inline static const uint32_t k_max_items = 1;

    private:
        std::unique_ptr<ComputePipeline> m_downsweep_pipeline;
        std::unique_ptr<ComputePipeline> m_workgroup_downsweep_pipeline;

    public:
        explicit BlellochScan();
        ~BlellochScan() = default;

        void operator()(
            std::shared_ptr<Buffer>& buffer,
            uint32_t length,
            uint32_t offset,
            uint32_t blocks_num,
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container
        );

    private:
        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            Buffer& buffer,
            uint32_t length,
            uint32_t offset
        );

        void downsweep(
            std::shared_ptr<Buffer>& buffer,
            uint32_t length,
            uint32_t offset,
            uint32_t blocks_num,
            bool clear_last,
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container
        );
    };
} // namespace vren
