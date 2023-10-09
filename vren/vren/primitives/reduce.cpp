#include "reduce.hpp"

#include <iostream>

#include <glm/gtc/integer.hpp>

#include "Toolbox.hpp"
#include "vk_helpers/misc.hpp"

template <typename _data_type_t, vren::reduce_operation _operation_t> char const* get_shader_filepath();

template <> char const* get_shader_filepath<glm::vec4, vren::ReduceOperationAdd>()
{
    return ".vren/resources/shaders/reduce_vec4_add.comp.spv";
};
template <> char const* get_shader_filepath<glm::vec4, vren::ReduceOperationMin>()
{
    return ".vren/resources/shaders/reduce_vec4_min.comp.spv";
};
template <> char const* get_shader_filepath<glm::vec4, vren::ReduceOperationMax>()
{
    return ".vren/resources/shaders/reduce_vec4_max.comp.spv";
};
template <> char const* get_shader_filepath<glm::uint, vren::ReduceOperationAdd>()
{
    return ".vren/resources/shaders/reduce_uint_add.comp.spv";
};
template <> char const* get_shader_filepath<glm::uint, vren::ReduceOperationMin>()
{
    return ".vren/resources/shaders/reduce_uint_min.comp.spv";
};
template <> char const* get_shader_filepath<glm::uint, vren::ReduceOperationMax>()
{
    return ".vren/resources/shaders/reduce_uint_max.comp.spv";
};

template <typename _data_type_t, vren::reduce_operation _operation_t>
vren::reduce<_data_type_t, _operation_t>::reduce(vren::context const& context) :
    m_context(&context),
    m_pipeline(
        [&]()
        {
            vren::shader_module shader_module =
                vren::load_shader_module_from_file(context, get_shader_filepath<_data_type_t, _operation_t>());
            vren::specialized_shader shader = vren::specialized_shader(shader_module);
            return vren::create_compute_pipeline(context, shader);
        }()
    )
{
}

template <typename _data_type_t, vren::reduce_operation _operation_t>
void vren::reduce<_data_type_t, _operation_t>::operator()(
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container,
    vren::vk_utils::buffer const& input_buffer,
    uint32_t input_buffer_length,
    size_t input_buffer_offset,
    vren::vk_utils::buffer const& output_buffer,
    size_t output_buffer_offset,
    uint32_t blocks_num
)
{
    uint32_t length_power_of_2 = vren::round_to_next_power_of_2(input_buffer_length);

    size_t output_buffer_size = length_power_of_2 * sizeof(_data_type_t);

    assert(output_buffer.m_allocation_info.size >= output_buffer_size);
    assert(blocks_num > 0);

    VkBufferMemoryBarrier buffer_memory_barrier{};
    VkBufferCopy region{};

    struct
    {
        uint32_t m_base_level;
        uint32_t m_input_length;
        uint32_t m_output_length;
    } push_constants;

    VkDescriptorSetLayout descriptor_set_layout = m_pipeline.m_descriptor_set_layouts.at(0);

    // Descriptor set used for the first dispatch to write data from InputBuffer to OutputBuffer
    auto descriptor_set_1 = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(descriptor_set_layout)
    );

    vren::vk_utils::write_buffer_descriptor(
        *m_context,
        descriptor_set_1->m_handle.m_descriptor_set,
        0,
        input_buffer.m_buffer.m_handle,
        input_buffer_length * sizeof(_data_type_t),
        input_buffer_offset
    );
    vren::vk_utils::write_buffer_descriptor(
        *m_context,
        descriptor_set_1->m_handle.m_descriptor_set,
        1,
        output_buffer.m_buffer.m_handle,
        output_buffer_size,
        output_buffer_offset
    );

    // Descriptor set used after the first dispatch that only use the OutputBuffer
    auto descriptor_set_2 = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(descriptor_set_layout)
    );

    vren::vk_utils::write_buffer_descriptor(
        *m_context,
        descriptor_set_2->m_handle.m_descriptor_set,
        0,
        output_buffer.m_buffer.m_handle,
        output_buffer_size,
        output_buffer_offset
    );
    vren::vk_utils::write_buffer_descriptor(
        *m_context,
        descriptor_set_2->m_handle.m_descriptor_set,
        1,
        output_buffer.m_buffer.m_handle,
        output_buffer_size,
        output_buffer_offset
    );

    resource_container.add_resources(descriptor_set_1, descriptor_set_2);

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
                .buffer = output_buffer.m_buffer.m_handle,
                .offset = output_buffer_offset,
                .size = output_buffer_size};
            vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                NULL,
                0,
                nullptr,
                1,
                &buffer_memory_barrier,
                0,
                nullptr
            );
        }

        m_pipeline.bind(command_buffer);

        push_constants = {
            .m_base_level = level,
            .m_input_length = level == 0 ? input_buffer_length : length_power_of_2,
            .m_output_length = length_power_of_2,
        };
        m_pipeline.push_constants(
            command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0
        );

        VkDescriptorSet descriptor_set =
            level == 0 ? descriptor_set_1->m_handle.m_descriptor_set : descriptor_set_2->m_handle.m_descriptor_set;
        m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(1 << (levels - level), k_workgroup_size);
        m_pipeline.dispatch(command_buffer, workgroups_num, blocks_num, 1);
    }
}

template <typename _data_type_t, vren::reduce_operation _operation_t>
void vren::reduce<_data_type_t, _operation_t>::operator()(
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    size_t offset,
    uint32_t blocks_num
)
{
    // InputBuffer is the same of the OutputBuffer (this is legal for the reduction operation)
    this->operator()(command_buffer, resource_container, buffer, length, offset, buffer, offset, blocks_num);
}

template class vren::reduce<glm::vec4, vren::ReduceOperationAdd>;
template class vren::reduce<glm::vec4, vren::ReduceOperationMin>;
template class vren::reduce<glm::vec4, vren::ReduceOperationMax>;
template class vren::reduce<glm::uint, vren::ReduceOperationAdd>;
template class vren::reduce<glm::uint, vren::ReduceOperationMin>;
template class vren::reduce<glm::uint, vren::ReduceOperationMax>;

uint32_t vren::calc_reduce_output_buffer_length(uint32_t count)
{
    return vren::round_to_next_power_of_2(count);
}
