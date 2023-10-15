#include "BlellochScan.hpp"

#include <iostream>

#include <fmt/format.h>
#include <glm/gtc/integer.hpp>

#include "Toolbox.hpp"
#include "vk_api/shader/Shader.hpp"
#include "vk_api/shader/ComputePipeline.hpp"

using namespace vren;

BlellochScan::BlellochScan()
{
    { // Downsweep
        std::shared_ptr<Shader> shader =
            ShaderBuilder(".vren/resources/shaders/blelloch_scan_downsweep.comp", VK_SHADER_STAGE_COMPUTE_BIT).build();
        m_downsweep_pipeline = std::make_unique<ComputePipeline>(shader);
    }

    { // Workgroup downsweep
        std::shared_ptr<Shader> shader =
            ShaderBuilder(".vren/resources/shaders/blelloch_scan_workgroup_downsweep.comp", VK_SHADER_STAGE_COMPUTE_BIT).build();
        m_workgroup_downsweep_pipeline = std::make_unique<ComputePipeline>(shader);
    }
}

void BlellochScan::write_descriptor_set(
    VkDescriptorSet descriptor_set,
    Buffer& buffer,
    uint32_t length,
    uint32_t offset
)
{
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer.buffer();
    buffer_info.offset = offset;
    buffer_info.range = VK_WHOLE_SIZE;

   VkWriteDescriptorSet descriptor_set_write{};
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
        .pTexelBufferView = nullptr};
    vkUpdateDescriptorSets(Context::get().device().handle(), 1, &descriptor_set_write, 0, nullptr);
}

void BlellochScan::downsweep(
    std::shared_ptr<Buffer>& buffer,
    uint32_t length,
    uint32_t offset,
    uint32_t blocks_num,
    bool clear_last,
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container
)
{
    assert(vren::is_power_of_2(length));
    assert(blocks_num > 0);

    struct
    {
        uint32_t m_level;
        uint32_t m_clear_last;
        uint32_t m_block_length;
        uint32_t m_num_items;
    } push_constants;

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_downsweep_pipeline->m_descriptor_set_layouts.at(0))
    );
    resource_container.add_resource(descriptor_set);
    write_descriptor_set(descriptor_set->m_handle.m_descriptor_set, buffer, length, offset);

    uint32_t num_items = 1; // TODO num_items could be dynamically calculated
    assert(num_items <= k_max_items);

    int32_t levels_per_workgroup = glm::log2<int32_t>(k_workgroup_size * num_items);

    // Downsweep (on global memory)
    for (int32_t level = glm::log2(length) - 1, i = 0; level > levels_per_workgroup - 1; level--, i++)
    {
        m_downsweep_pipeline->bind(command_buffer);

        push_constants = {
            .m_level = (uint32_t) level,
            .m_clear_last = clear_last ? 1u : 0,
            .m_block_length = length,
            .m_num_items = num_items,
        };
        m_downsweep_pipeline->push_constants(
            command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0
        );

        m_downsweep_pipeline->bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(1 << i, k_workgroup_size * num_items);
        m_downsweep_pipeline->dispatch(workgroups_num, blocks_num, 1, command_buffer, resource_container);

        VkBufferMemoryBarrier buffer_memory_barrier{};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        buffer_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        buffer_memory_barrier.buffer = buffer->buffer();
        buffer_memory_barrier.offset = offset;
        buffer_memory_barrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            NULL,
            0, nullptr,
            1, &buffer_memory_barrier,
            0, nullptr
        );

        if (clear_last)
            clear_last = false;
    }

    // Workgroup downsweep
    m_workgroup_downsweep_pipeline->bind(command_buffer);

    push_constants = {
        .m_level = 0, // Ignored
        .m_clear_last = clear_last ? 1u : 0,
        .m_block_length = length,
        .m_num_items = num_items,
    };
    m_workgroup_downsweep_pipeline->push_constants(
        command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0
    );

    m_workgroup_downsweep_pipeline->bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    uint32_t workgroups_num = vren::divide_and_ceil(length, k_workgroup_size * num_items);
    m_workgroup_downsweep_pipeline->dispatch(command_buffer, workgroups_num, blocks_num, 1);
}

void BlellochScan::operator()(
    std::shared_ptr<Buffer>& buffer,
    uint32_t length,
    uint32_t offset,
    uint32_t blocks_num,
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container
)
{
    // Reduce
    Reduce::get(Reduce::DataType::UINT, Reduce::Operation::ADD)(buffer, length, offset, 1, command_buffer, resource_container);

    VkBufferMemoryBarrier buffer_memory_barrier{};
    buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buffer_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    buffer_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    buffer_memory_barrier.buffer = buffer->buffer();
    buffer_memory_barrier.offset = offset;
    buffer_memory_barrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        NULL,
        0, nullptr,
        1, &buffer_memory_barrier,
        0, nullptr
    );

    // Clear last + downsweep
    downsweep(buffer, length, offset, blocks_num, /* Clear last */ true, command_buffer, resource_container);
}
