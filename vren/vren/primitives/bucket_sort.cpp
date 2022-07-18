#include "bucket_sort.hpp"

#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"

vren::bucket_sort::bucket_sort(vren::context const& context) :
    m_context(&context),
    m_descriptor_set_layout(create_descriptor_set_layout()),
    m_count_pipeline([&]()
    {
        vren::shader_module shader_mod = vren::load_shader_module_from_file(context, ".vren/resources/shaders/bucket_sort_count.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_mod);
        return vren::create_compute_pipeline(context, shader);
    }()),
    m_write_pipeline([&]()
    {
        vren::shader_module shader_mod = vren::load_shader_module_from_file(context, ".vren/resources/shaders/bucket_sort_write.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_mod);
        return vren::create_compute_pipeline(context, shader);
    }())
{}

vren::vk_descriptor_set_layout vren::bucket_sort::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding bindings[]{
        { // Input buffer
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr
        },
        { // Output buffer
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr
        },
        { // Bucket count/offset buffer
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr
        },
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .bindingCount = std::size(bindings),
        .pBindings = bindings
    };
    VkDescriptorSetLayout descriptor_set_layout;
    VREN_CHECK(vkCreateDescriptorSetLayout(m_context->m_device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout), m_context);
    return vren::vk_descriptor_set_layout(*m_context, descriptor_set_layout);
}

void vren::bucket_sort::write_descriptor_set(
    VkDescriptorSet descriptor_set,
    vren::vk_utils::buffer const& input_buffer,
    uint32_t length,
    vren::vk_utils::buffer const& scratch_buffer_1
)
{
    VkDescriptorBufferInfo buffer_infos[]{
        { // Input buffer
            .buffer = input_buffer.m_buffer.m_handle,
            .offset = 0,
            .range = length * sizeof(glm::uvec2),
        },
        { // Output buffer
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .range = length * sizeof(glm::uvec2),
        },
        { // Bucket count/offset buffer
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = length * sizeof(glm::uvec2),
            .range = k_key_size * sizeof(uint32_t),
        }
    };

    VkWriteDescriptorSet descriptor_set_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = std::size(buffer_infos),
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = buffer_infos,
        .pTexelBufferView = nullptr
    };
    vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

vren::vk_utils::buffer vren::bucket_sort::create_scratch_buffer_1(uint32_t length)
{
    auto scratch_buffer_1 =
        vren::vk_utils::alloc_device_only_buffer(
            *m_context,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            length * sizeof(glm::uvec2) + k_key_size * sizeof(uint32_t)
        );
    return scratch_buffer_1;
}

void vren::bucket_sort::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    vren::vk_utils::buffer const& scratch_buffer_1
)
{
    const uint32_t num_items = 1;
    const uint32_t num_workgroups = vren::divide_and_ceil(length, k_workgroup_size * num_items);

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_descriptor_set_layout.m_handle)
    );
    resource_container.add_resource(descriptor_set);
    write_descriptor_set(descriptor_set->m_handle.m_descriptor_set, buffer, length, scratch_buffer_1);

    VkBufferMemoryBarrier buffer_memory_barrier{};

    // Clear
    vkCmdFillBuffer(
        command_buffer,
        scratch_buffer_1.m_buffer.m_handle,
        length * sizeof(glm::uvec2),
        k_key_size * sizeof(uint32_t),
        0
    );

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = scratch_buffer_1.m_buffer.m_handle,
        .offset = length * sizeof(glm::uvec2),
        .size = k_key_size * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Per-bucket count
    m_count_pipeline.bind(command_buffer);

    m_count_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &num_items, sizeof(num_items), 0);
    m_count_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    m_count_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = scratch_buffer_1.m_buffer.m_handle,
        .offset = length * sizeof(glm::uvec2),
        .size = k_key_size * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Bucket offsets (exclusive scan)
    m_context->m_toolbox->m_blelloch_scan(
        command_buffer,
        resource_container,
        scratch_buffer_1,
        k_key_size,
        length * sizeof(glm::uvec2),
        1
    );

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = scratch_buffer_1.m_buffer.m_handle,
        .offset = length * sizeof(glm::uvec2),
        .size = k_key_size * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Write
    m_write_pipeline.bind(command_buffer);

    m_write_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &num_items, sizeof(num_items), 0);
    m_write_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    m_write_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .buffer = scratch_buffer_1.m_buffer.m_handle,
        .offset = 0,
        .size = length * sizeof(glm::uvec2)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Copy to input buffer
    VkBufferCopy region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = length * sizeof(glm::uvec2),
    };
    vkCmdCopyBuffer(command_buffer, scratch_buffer_1.m_buffer.m_handle, buffer.m_buffer.m_handle, 1, &region);
}
