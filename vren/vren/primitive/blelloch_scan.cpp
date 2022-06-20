#include "blelloch_scan.hpp"
#include "toolbox.hpp"

#include <glm/gtc/integer.hpp>

vren::blelloch_scan::blelloch_scan(vren::context const& context) :
    m_context(&context),
    m_upsweep_pipeline([&]()
    {
        vren::shader_module shader_mod = vren::load_shader_module_from_file(context, ".vren/resources/shaders/blelloch_scan_upsweep.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_mod);
        return vren::create_compute_pipeline(context, shader);
    }()),
    m_downsweep_pipeline([&]()
    {
        vren::shader_module shader_mod = vren::load_shader_module_from_file(context, ".vren/resources/shaders/blelloch_scan_downsweep.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_mod);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::blelloch_scan::write_descriptor_set(VkDescriptorSet descriptor_set, vren::vk_utils::buffer const& buffer)
{
    VkWriteDescriptorSet descriptor_set_write{};
    VkDescriptorBufferInfo buffer_info{};

    buffer_info = {
        .buffer = buffer.m_buffer.m_handle,
        .offset = 0,
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

void vren::blelloch_scan::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    uint32_t stride,
    uint32_t offset
)
{
    const uint32_t k_workgroup_size = 32;
    const uint32_t k_iters_num = 4;
    
    assert(vren::is_power_of_2(length));
    assert(stride > 0);

    struct
    {
        uint32_t m_offset;
        uint32_t m_stride;
    } push_constants;

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_upsweep_pipeline.m_descriptor_set_layouts.at(0))
    );
    resource_container.add_resource(descriptor_set);
    write_descriptor_set(descriptor_set->m_handle.m_descriptor_set, buffer);

    VkBufferMemoryBarrier buffer_memory_barrier{};

    // Upsweep
    for (uint32_t level = 0; level < glm::log2(length); level++)
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
                .size = length * sizeof(uint32_t)
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        }

        m_upsweep_pipeline.bind(command_buffer);

        push_constants = {
            .m_offset = offset + (1 << level) - 1,
            .m_stride = stride << level
        };
        m_upsweep_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        m_upsweep_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(length, k_workgroup_size * k_iters_num);
        m_upsweep_pipeline.dispatch(command_buffer, workgroups_num, 1, 1);
    }

    // Clear last
    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .buffer = buffer.m_buffer.m_handle,
        .offset = offset,
        .size = length * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    vkCmdFillBuffer(command_buffer, buffer.m_buffer.m_handle, offset + (length - 1) * sizeof(uint32_t), sizeof(uint32_t), 0);

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .buffer = buffer.m_buffer.m_handle,
        .offset = offset,
        .size = length * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Downsweep
    for (int32_t level = glm::log2(length) - 1; level >= 0; level--)
    {
        m_downsweep_pipeline.bind(command_buffer);

        push_constants = {
             .m_offset = offset + (1 << level) - 1,
             .m_stride = stride << level
        };
        m_downsweep_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        m_downsweep_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        uint32_t workgroups_num = vren::divide_and_ceil(length, k_workgroup_size * k_iters_num);
        m_downsweep_pipeline.dispatch(command_buffer, workgroups_num, 1, 1);

        if (level > 0)
        {
            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .buffer = buffer.m_buffer.m_handle,
                .offset = offset,
                .size = length * sizeof(uint32_t)
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        }
    }
}

