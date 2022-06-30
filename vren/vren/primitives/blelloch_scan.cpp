#include "blelloch_scan.hpp"

#include <iostream>

#include <glm/gtc/integer.hpp>

#include "toolbox.hpp"

vren::blelloch_scan::blelloch_scan(vren::context const& context) :
    m_context(&context),
    m_downsweep_pipeline([&]()
    {
        vren::shader_module shader_mod = vren::load_shader_module_from_file(context, ".vren/resources/shaders/blelloch_scan_downsweep.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_mod);
        return vren::create_compute_pipeline(context, shader);
    }()),
    m_subgroup_downsweep_pipeline([&]()
    {
        vren::shader_module shader_mod = vren::load_shader_module_from_file(context, ".vren/resources/shaders/blelloch_scan_subgroup_downsweep.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_mod);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::blelloch_scan::write_descriptor_set(
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
        .range = length * sizeof(uint32_t)
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

void vren::blelloch_scan::downsweep(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    uint32_t offset,
    uint32_t block_num,
    bool clear_last
)
{
    assert(vren::is_power_of_2(length));
    assert(length >= k_min_buffer_length);

    struct
    {
        uint32_t m_offset;
        uint32_t m_stride;
        uint32_t m_clear_last;
    } push_constants;

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_downsweep_pipeline.m_descriptor_set_layouts.at(0))
    );
    resource_container.add_resource(descriptor_set);
    write_descriptor_set(descriptor_set->m_handle.m_descriptor_set, buffer, length, offset);

    int32_t levels_per_subgroup = glm::log2<int32_t>(k_downsweep_subgroup_size * k_downsweep_num_iterations);

    // Downsweep (on global memory)
    for (int32_t level = glm::log2(length) - 1, i = 0; level > levels_per_subgroup; level--, i++)
    {
        m_downsweep_pipeline.bind(command_buffer);

        push_constants = {
             .m_offset = (1u << level) - 1,
             .m_stride = 1u << level,
             .m_clear_last = clear_last ? 1u : 0
        };
        m_downsweep_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        m_downsweep_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(1 << i, k_downsweep_subgroup_size * k_downsweep_num_iterations);

        m_downsweep_pipeline.dispatch(command_buffer, workgroups_num, 1, 1);

        VkBufferMemoryBarrier buffer_memory_barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .buffer = buffer.m_buffer.m_handle,
            .offset = offset,
            .size = length * sizeof(uint32_t)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        if (clear_last)
        {
            clear_last = false;
        }
    }
    
    // Subgroup downsweep (per-subgroup: efficient)
    m_subgroup_downsweep_pipeline.bind(command_buffer);

    push_constants = {
        .m_offset = 0, // Ignored
        .m_stride = 1, // Ignored
        .m_clear_last = clear_last ? 1u : 0
    };
    m_subgroup_downsweep_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

    m_subgroup_downsweep_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    uint32_t workgroups_num = vren::divide_and_ceil(length, k_subgroup_downsweep_subgroup_size * k_subgroup_downseep_num_iterations);

    m_subgroup_downsweep_pipeline.dispatch(command_buffer, workgroups_num, 1, 1);
}

void vren::blelloch_scan::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    uint32_t offset
)
{
    // Reduce
    m_context->m_toolbox->m_reduce(command_buffer, resource_container, buffer, length, offset, 1, nullptr);

    VkBufferMemoryBarrier buffer_memory_barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .buffer = buffer.m_buffer.m_handle,
        .offset = offset,
        .size = length * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Clear last + downsweep
    downsweep(command_buffer, resource_container, buffer, length, offset, /* Clear last */ true);
}
