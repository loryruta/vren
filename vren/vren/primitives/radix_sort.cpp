#include "radix_sort.hpp"

#include "Context.hpp"
#include "Toolbox.hpp"
#include "vk_api/utils.hpp"

vren::radix_sort::radix_sort(vren::context const& context) :
    m_context(&context),
    m_descriptor_set_layout(create_descriptor_set_layout()),
    m_local_count_pipeline(
        [&]()
        {
            vren::shader_module shader_mod =
                vren::load_shader_module_from_file(context, ".vren/resources/shaders/radix_sort_local_count.comp.spv");
            vren::specialized_shader shader = vren::specialized_shader(shader_mod);
            return vren::create_compute_pipeline(context, shader);
        }()
    ),
    m_global_offset_pipeline(
        [&]()
        {
            vren::shader_module shader_mod = vren::load_shader_module_from_file(
                context, ".vren/resources/shaders/radix_sort_global_offset.comp.spv"
            );
            vren::specialized_shader shader = vren::specialized_shader(shader_mod);
            return vren::create_compute_pipeline(context, shader);
        }()
    ),
    m_reorder_pipeline(
        [&]()
        {
            vren::shader_module shader_mod =
                vren::load_shader_module_from_file(context, ".vren/resources/shaders/radix_sort_reorder.comp.spv");
            vren::specialized_shader shader = vren::specialized_shader(shader_mod);
            return vren::create_compute_pipeline(context, shader);
        }()
    )
{
}

vren::vk_descriptor_set_layout vren::radix_sort::create_descriptor_set_layout()
{
    VkDescriptorSetLayoutBinding bindings[]{
        {// Input buffer
         .binding = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_ALL,
         .pImmutableSamplers = nullptr},
        {// Output buffer
         .binding = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_ALL,
         .pImmutableSamplers = nullptr},
        {// Local offset buffer
         .binding = 2,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_ALL,
         .pImmutableSamplers = nullptr},
        {// Global offset buffer
         .binding = 3,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_ALL,
         .pImmutableSamplers = nullptr}};

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .bindingCount = std::size(bindings),
        .pBindings = bindings};
    VkDescriptorSetLayout descriptor_set_layout;
    VREN_CHECK(
        vkCreateDescriptorSetLayout(m_context->m_device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout),
        m_context
    );
    return vren::vk_descriptor_set_layout(*m_context, descriptor_set_layout);
}

void vren::radix_sort::write_descriptor_set(
    VkDescriptorSet descriptor_set,
    vren::vk_utils::buffer const& input_buffer,
    vren::vk_utils::buffer const& output_buffer,
    uint32_t length,
    vren::vk_utils::buffer const& scratch_buffer_1
)
{
    uint32_t num_workgroups = vren::divide_and_ceil(length, k_workgroup_size); // * num_items = 1
    uint32_t local_offset_block_length = num_workgroups;

    VkDescriptorBufferInfo buffer_infos[]{
        {
            // Input buffer
            .buffer = input_buffer.m_buffer.m_handle,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        },
        {
            // Output buffer
            .buffer = output_buffer.m_buffer.m_handle,
            .offset = 0,
            .range = VK_WHOLE_SIZE,
        },
        {
            // Local offset buffer
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .range = local_offset_block_length * 16 * sizeof(uint32_t),
        },
        {
            // Global offset buffer
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = local_offset_block_length * 16 * sizeof(uint32_t),
            .range = 16 * sizeof(uint32_t),
        },
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
        .pTexelBufferView = nullptr};
    vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

vren::vk_utils::buffer vren::radix_sort::create_scratch_buffer_1(uint32_t length)
{
    uint32_t num_workgroups = vren::divide_and_ceil(length, k_workgroup_size); // * num_items = 1
    uint32_t local_offset_block_length = num_workgroups;

    auto scratch_buffer_1 = vren::vk_utils::alloc_device_only_buffer(
        *m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        (local_offset_block_length * 16 + 16) * sizeof(uint32_t)
    );
    return scratch_buffer_1;
}

vren::vk_utils::buffer vren::radix_sort::create_scratch_buffer_2(uint32_t length)
{
    auto scratch_buffer_2 = vren::vk_utils::alloc_device_only_buffer(
        *m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, length * sizeof(uint32_t)
    );
    return scratch_buffer_2;
}

void vren::radix_sort::operator()(
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t length,
    vren::vk_utils::buffer const& scratch_buffer_1,
    vren::vk_utils::buffer const& scratch_buffer_2
)
{
    if (!(length >= k_workgroup_size && vren::is_power_of_2(length)))
    {
        throw std::invalid_argument("Length must be higher than 1024 and a power of 2");
    }

    VkBufferMemoryBarrier buffer_memory_barrier{};

    uint32_t num_items = 1; // TODO calculate dynamically
    assert(num_items <= k_max_items);

    uint32_t num_workgroups = vren::divide_and_ceil(length, k_workgroup_size); // * num_items = 1
    uint32_t local_offset_block_length = num_workgroups;

    for (int32_t i = 0; i < 8; i++)
    {
        vren::vk_utils::buffer const& input_buffer = i % 2 == 0 ? buffer : scratch_buffer_2;
        vren::vk_utils::buffer const& output_buffer = i % 2 == 0 ? scratch_buffer_2 : buffer;

        auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_descriptor_set_layout.m_handle)
        );
        resource_container.add_resource(descriptor_set);
        write_descriptor_set(
            descriptor_set->m_handle.m_descriptor_set, input_buffer, output_buffer, length, scratch_buffer_1
        );

        // Clear scratch buffer 1 to zero!
        vkCmdFillBuffer(command_buffer, scratch_buffer_1.m_buffer.m_handle, 0, VK_WHOLE_SIZE, 0);

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE};
        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            NULL,
            0,
            nullptr,
            1,
            &buffer_memory_barrier,
            0,
            nullptr
        );

        // Local count
        {
            m_local_count_pipeline.bind(command_buffer);

            m_local_count_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

            struct
            {
                uint32_t m_symbol_position;
                uint32_t m_block_length;
                uint32_t m_num_items;
            } push_constants;

            push_constants.m_symbol_position = i;
            push_constants.m_block_length = local_offset_block_length;
            push_constants.m_num_items = num_items;

            m_local_count_pipeline.push_constants(
                command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants)
            );

            m_local_count_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);
        }

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE};
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

        // Reduce local counts to obtain scattered global counts
        m_context->m_toolbox->m_reduce_uint_add(
            command_buffer, resource_container, scratch_buffer_1, local_offset_block_length, 0, 16
        );

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = (local_offset_block_length * 16) * sizeof(uint32_t)};
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

        // Copy global counts and exclusive-scan for global offsets
        {
            m_global_offset_pipeline.bind(command_buffer);

            m_global_offset_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

            struct
            {
                uint32_t m_offset;
                uint32_t m_stride;
            } push_constants;

            push_constants.m_offset = local_offset_block_length - 1;
            push_constants.m_stride = local_offset_block_length;

            m_global_offset_pipeline.push_constants(
                command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants)
            );

            m_global_offset_pipeline.dispatch(command_buffer, 1, 1, 1);
        }

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = (local_offset_block_length * 16) * sizeof(uint32_t),
            .size = 16 * sizeof(uint32_t)};
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

        // Downsweep on reduced local counts (therefore blelloch scan to obtain local offsets)
        m_context->m_toolbox->m_blelloch_scan.downsweep(
            command_buffer,
            resource_container,
            scratch_buffer_1,
            local_offset_block_length,
            0,
            16,
            true // Clear last
        );

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = (local_offset_block_length * 16) * sizeof(uint32_t),
        };
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

        // Reordering
        {
            m_reorder_pipeline.bind(command_buffer);

            m_reorder_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

            struct
            {
                uint32_t m_symbol_position;
                uint32_t m_local_offset_block_length;
                uint32_t m_num_items;
            } push_constants;

            push_constants.m_symbol_position = i;
            push_constants.m_local_offset_block_length = local_offset_block_length;
            push_constants.m_num_items = num_items;

            m_reorder_pipeline.push_constants(
                command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants)
            );

            m_reorder_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);
        }

        if (i < (8 - 1))
        {
            buffer_memory_barrier = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .buffer = output_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE};
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
    }
}
