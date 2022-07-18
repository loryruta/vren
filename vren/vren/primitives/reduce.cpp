#include "reduce.hpp"

#include <iostream>

#include <glm/gtc/integer.hpp>

#include "toolbox.hpp"

std::string build_shader_path(vren::reduce_data_type::enum_t data_type, vren::reduce_operation::enum_t operation)
{
    auto result = std::string(".vren/resources/shaders/reduce_");
    result += std::string( vren::reduce_data_type::get_name(data_type)) + "_" + vren::reduce_operation::get_name(operation);
    result += ".comp.spv";
    return result;
}

vren::reduce::reduce(
    vren::context const& context,
    vren::reduce_data_type::enum_t data_type,
    vren::reduce_operation::enum_t operation
) :
    m_context(&context),
    m_pipeline([&]()
    {
        std::string shader_path = build_shader_path(data_type, operation);
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, shader_path.c_str());
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::reduce::write_descriptor_set(
    VkDescriptorSet descriptor_set,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    uint32_t offset
)
{
    VkWriteDescriptorSet descriptor_set_write{};
    VkDescriptorBufferInfo buffer_info{};

    buffer_info = {
        .buffer = buffer.m_buffer.m_handle,
        .offset = offset,
        .range = VK_WHOLE_SIZE
    };
    descriptor_set_write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_info,
        .pTexelBufferView = nullptr
    };
    vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::reduce::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    uint32_t offset,
    uint32_t blocks_num,
    uint32_t* result
)
{
    assert(vren::is_power_of_2(length));
    assert(blocks_num > 0);

    struct
    {
        uint32_t m_num_items;
        uint32_t m_base_level;
        uint32_t m_block_length;
    } push_constants;

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
    );
    resource_container.add_resource(descriptor_set);
    write_descriptor_set(descriptor_set->m_handle.m_descriptor_set, buffer, length, offset);

    VkBufferMemoryBarrier buffer_memory_barrier{};

    uint32_t num_items = 1;
    int32_t max_levels_per_dispatch = glm::log2<int32_t>(k_workgroup_size * num_items);
    int32_t levels = glm::log2<int32_t>(length);

    for (uint32_t level = 0; level < levels; level += max_levels_per_dispatch)
    {
        if (level > 0)
        {
            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .buffer = buffer.m_buffer.m_handle,
                .offset = offset,
                .size = VK_WHOLE_SIZE
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        }

        m_pipeline.bind(command_buffer);

        push_constants = {
            .m_num_items = num_items,
            .m_base_level = level,
            .m_block_length = length,
        };
        m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(1 << (levels - level), k_workgroup_size * num_items);
        m_pipeline.dispatch(command_buffer, workgroups_num, blocks_num, 1);
    }

    if (result)
    {
        // TODO write to `result` if set
    }
}
