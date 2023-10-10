#pragma once

#include "vk_api/buffer/buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    enum reduce_operation
    {
        ReduceOperationAdd,
        ReduceOperationMin,
        ReduceOperationMax
    };

    template <typename _data_type_t, vren::reduce_operation _operation_t> class reduce
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;

    private:
        vren::context const* m_context;
        vren::pipeline m_pipeline;

    public:
        reduce(vren::context const& context);

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& input_buffer,
            uint32_t input_buffer_length,
            size_t input_buffer_offset,
            vren::vk_utils::buffer const& output_buffer,
            size_t output_buffer_offset,
            uint32_t blocks_num
        );

        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            size_t offset,
            uint32_t blocks_num
        );
    };

    uint32_t calc_reduce_output_buffer_length(uint32_t count);
} // namespace vren
