#include "bucket_sort.hpp"

#include "context.hpp"
#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"

vren::bucket_sort::bucket_sort(vren::context const& context) :
    m_context(&context),
    m_descriptor_set_layout(create_descriptor_set_layout()),
    m_count_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/bucket_sort_count.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }()),
    m_write_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/bucket_sort_write.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
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

VkBufferUsageFlags vren::bucket_sort::get_required_output_buffer_usage_flags()
{
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
}

size_t vren::bucket_sort::get_required_output_buffer_size(uint32_t length)
{
    return vren::round_to_next_multiple_of(length * sizeof(glm::uvec2), (size_t) VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT) + k_key_size * sizeof(uint32_t);
}

void vren::bucket_sort::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& input_buffer,
    uint32_t input_buffer_length,
    size_t input_buffer_offset,
    vren::vk_utils::buffer const& output_buffer,
    size_t output_buffer_offset
)
{
    assert(input_buffer_offset % VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT == 0);
    assert(output_buffer_offset % VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT == 0);
    assert((output_buffer.m_allocation_info.size - output_buffer_offset) >= get_required_output_buffer_size(input_buffer_length));

    size_t bucket_count_buffer_offset = output_buffer_offset + vren::round_to_next_multiple_of(input_buffer_length * sizeof(glm::uvec2), (size_t) VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT);

    const uint32_t num_workgroups = vren::divide_and_ceil(input_buffer_length, k_workgroup_size);

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_descriptor_set_layout.m_handle)
    );

    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, input_buffer.m_buffer.m_handle, input_buffer_length * sizeof(glm::uvec2), input_buffer_offset);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, output_buffer.m_buffer.m_handle, input_buffer_length * sizeof(glm::uvec2), output_buffer_offset);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 2, output_buffer.m_buffer.m_handle, k_key_size * sizeof(uint32_t), bucket_count_buffer_offset);

    resource_container.add_resource(descriptor_set);

    VkBufferMemoryBarrier buffer_memory_barrier{};

    // Clear
    vkCmdFillBuffer(command_buffer, output_buffer.m_buffer.m_handle, bucket_count_buffer_offset, k_key_size * sizeof(uint32_t), 0);

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = output_buffer.m_buffer.m_handle,
        .offset = bucket_count_buffer_offset,
        .size = k_key_size * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Per-bucket count
    m_count_pipeline.bind(command_buffer);

    m_count_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    m_count_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = output_buffer.m_buffer.m_handle,
        .offset = bucket_count_buffer_offset,
        .size = k_key_size * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Bucket offsets (exclusive scan)
    m_context->m_toolbox->m_blelloch_scan(
        command_buffer,
        resource_container,
        output_buffer,
        k_key_size,
        bucket_count_buffer_offset,
        1
    );

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = output_buffer.m_buffer.m_handle,
        .offset = bucket_count_buffer_offset,
        .size = k_key_size * sizeof(uint32_t)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // Write
    m_write_pipeline.bind(command_buffer);

    m_write_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    m_write_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);
}
