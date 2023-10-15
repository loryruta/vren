#include "Reduce.hpp"

#include <array>
#include <iostream>

#include <glm/gtc/integer.hpp>

#include "Toolbox.hpp"
#include "vk_api/DescriptorPool.hpp"
#include "vk_api/misc_utils.hpp"

using namespace vren;

namespace
{
    std::unordered_map<uint32_t, std::unique_ptr<Reduce>> s_reduce_map;

    char const* data_type_macro_value(Reduce::DataType data_type)
    {
        switch (data_type)
        {
        case Reduce::DataType::UINT:
            return "uint";
        case Reduce::DataType::VEC4:
            return "vec4";
        default:
            throw std::runtime_error("Invalid data type");
        }
    }

    size_t data_type_size(Reduce::DataType data_type)
    {
        switch (data_type)
        {
        case Reduce::DataType::UINT:
            return sizeof(glm::uint);
        case Reduce::DataType::VEC4:
            return sizeof(glm::vec4);
        default:
            throw std::runtime_error("Invalid data type");
        }
    }

    char const* operation_macro_value(Reduce::Operation operation)
    {
        switch (operation)
        {
        case Reduce::Operation::ADD:
            return "(a + b)";
        case Reduce::Operation::MIN:
            return "min(a, b)";
        case Reduce::Operation::MAX:
            return "max(a, b)";
        default:
            throw std::runtime_error("Invalid operation");
        }
    }

    char const* out_of_bound_macro_value(Reduce::DataType data_type, Reduce::Operation operation)
    {
        // clang-format off
        if (data_type == Reduce::DataType::UINT && operation == Reduce::Operation::ADD) return "0u";
        if (data_type == Reduce::DataType::UINT && operation == Reduce::Operation::MIN) return "0u";
        if (data_type == Reduce::DataType::UINT && operation == Reduce::Operation::MAX) return "~0u";

        if (data_type == Reduce::DataType::VEC4 && operation == Reduce::Operation::ADD) return "vec4(0)";
        if (data_type == Reduce::DataType::VEC4 && operation == Reduce::Operation::MIN) return "vec4(1e35)";
        if (data_type == Reduce::DataType::VEC4 && operation == Reduce::Operation::MAX) return "vec4(-1e35)";
        // clang-format on

        throw std::runtime_error("VREN_OUT_OF_BOUND_VALUE not defined for (data_type, operation)");
    }
} // namespace

Reduce::Reduce(DataType data_type, Operation operation) :
    m_data_type(data_type),
    m_operation(operation)
{
    std::shared_ptr<Shader> shader = ShaderBuilder(".vren/resources/shaders/reduce.comp", VK_SHADER_STAGE_COMPUTE_BIT)
         .add_macro_definition("VREN_DATA_TYPE", data_type_macro_value(data_type))
         .add_macro_definition("VREN_OPERATION", operation_macro_value(operation))
         .add_macro_definition("VREN_OUT_OF_BOUND_VALUE", out_of_bound_macro_value(data_type, operation))
         .build();
    m_pipeline = std::make_unique<ComputePipeline>(shader);
}

void Reduce::operator()(
    std::shared_ptr<Buffer>& input_buffer,
    uint32_t input_buffer_length,
    size_t input_buffer_offset,
    std::shared_ptr<Buffer>& output_buffer,
    size_t output_buffer_offset,
    uint32_t blocks_num,
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container
)
{
    uint32_t length_power_of_2 = vren::round_to_next_power_of_2(input_buffer_length);

    size_t output_buffer_size = length_power_of_2 * data_type_size(m_data_type);

    assert(output_buffer->size() >= output_buffer_size);
    assert(blocks_num > 0);

    VkBufferMemoryBarrier buffer_memory_barrier{};
    VkBufferCopy region{};

    struct PushConstants
    {
        uint32_t m_base_level;
        uint32_t m_input_length;
        uint32_t m_output_length;
    };
    PushConstants push_constants{};

    VkDescriptorSetLayout descriptor_set_layout = m_pipeline->descriptor_set_layout(0);

    // Descriptor set used for the first dispatch to write data from InputBuffer to OutputBuffer
    auto pooled_set_1 = std::make_shared<PooledDescriptorSet>(DescriptorPool::get_default().acquire(descriptor_set_layout));

    write_buffer_descriptor(pooled_set_1->set(), 0, input_buffer->buffer(), input_buffer_length * data_type_size(m_data_type), input_buffer_offset);
    write_buffer_descriptor(pooled_set_1->set(), 1, output_buffer->buffer(), output_buffer_size, output_buffer_offset);

    // Descriptor set used after the first dispatch that only use the OutputBuffer
    auto pooled_set_2 = std::make_shared<PooledDescriptorSet>(DescriptorPool::get_default().acquire(descriptor_set_layout));

    write_buffer_descriptor(pooled_set_2->set(), 0, output_buffer->buffer(), output_buffer_size, output_buffer_offset);
    write_buffer_descriptor(pooled_set_2->set(), 1, output_buffer->buffer(), output_buffer_size, output_buffer_offset);

    resource_container.add_resources(pooled_set_1, pooled_set_2);

    int32_t max_levels_per_dispatch = glm::log2<int32_t>(k_workgroup_size);
    int32_t levels = glm::max(glm::log2<int32_t>(length_power_of_2), 1);

    for (uint32_t level = 0; level < levels; level += max_levels_per_dispatch)
    {
        if (level > 0)
        {
            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .buffer = output_buffer->buffer(),
                .offset = output_buffer_offset,
                .size = output_buffer_size};
            vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0,
                nullptr
            );
        }

        m_pipeline->bind(command_buffer);

        push_constants = {
            .m_base_level = level,
            .m_input_length = level == 0 ? input_buffer_length : length_power_of_2,
            .m_output_length = length_power_of_2,
        };
        m_pipeline->push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        VkDescriptorSet descriptor_set = level == 0 ? pooled_set_1->set() : pooled_set_2->set();
        m_pipeline->bind_descriptor_set(command_buffer, 0, descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(1 << (levels - level), k_workgroup_size);
        m_pipeline->dispatch(workgroups_num, blocks_num, 1, command_buffer, resource_container);
    }
}

void Reduce::operator()(
    std::shared_ptr<Buffer>& buffer,
    uint32_t length,
    size_t offset,
    uint32_t blocks_num,
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container
)
{
    // InputBuffer is the same of the OutputBuffer (this is legal for the reduction operation)
    operator()(buffer, length, offset, buffer, offset, blocks_num, command_buffer, resource_container);
}

Reduce& Reduce::get(DataType data_type, Operation operation)
{

}
