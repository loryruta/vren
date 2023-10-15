#pragma once

#include "vk_api/buffer/Buffer.hpp"
#include "vk_api/shader/ComputePipeline.hpp"

namespace vren
{
    class Reduce
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;

        enum class DataType : uint8_t
        {
            UINT = 0,
            VEC4,
            Count
        };

        enum class Operation : uint8_t
        {
            ADD = 0,
            MIN,
            MAX,
            Count
        };

    private:
        DataType m_data_type;
        Operation m_operation;

        std::unique_ptr<ComputePipeline> m_pipeline;

    public:
        explicit Reduce(DataType data_type, Operation operation);
        ~Reduce() = default;

        void operator()(
            std::shared_ptr<Buffer>& input_buffer,
            uint32_t input_buffer_length,
            size_t input_buffer_offset,
            std::shared_ptr<Buffer>& output_buffer,
            size_t output_buffer_offset,
            uint32_t blocks_num,
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container
        );

        void operator()(
            std::shared_ptr<Buffer>& buffer,
            uint32_t length,
            size_t offset,
            uint32_t blocks_num,
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container
        );

        static Reduce& get(DataType data_type, Operation operation);
    };
} // namespace vren
