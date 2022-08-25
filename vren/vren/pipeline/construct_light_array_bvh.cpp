#include "construct_light_array_bvh.hpp"

#include "toolbox.hpp"
#include "primitives/reduce.hpp"
#include "vk_helpers/misc.hpp"

vren::construct_light_array_bvh::construct_light_array_bvh(vren::context const& context) :
    m_context(&context),
    m_discretize_light_positions_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/discretize_light_positions.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }()),
    m_init_light_array_bvh_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/init_light_array_bvh.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

VkBufferUsageFlags vren::construct_light_array_bvh::get_required_bvh_buffer_usage_flags()
{
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
}

size_t vren::construct_light_array_bvh::get_required_bvh_buffer_size(uint32_t light_count) // scratch_buffer_1
{
    uint32_t light_count_power_of_2 = vren::round_to_next_power_of_2(light_count);

    return glm::max<size_t>(
        glm::max<size_t>(
            vren::build_bvh::get_required_buffer_size(light_count), // BVH
            light_count * sizeof(glm::uvec2) // Morton codes
        ),
        VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 3 + vren::round_to_next_multiple_of(light_count_power_of_2 * sizeof(glm::vec4), VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT) + 2 * sizeof(glm::vec4) // Reduction + min/max
    );
}

VkBufferUsageFlags vren::construct_light_array_bvh::get_required_light_index_buffer_usage_flags()
{
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | vren::bucket_sort::get_required_output_buffer_usage_flags();
}

size_t vren::construct_light_array_bvh::get_required_light_index_buffer_size(uint32_t light_count) // scratch_buffer_2
{
    return glm::max<size_t>(
        light_count * sizeof(glm::uvec2),
        (light_count + 2) * sizeof(glm::vec4)
    );
}

vren::render_graph_t vren::construct_light_array_bvh::construct(
    vren::render_graph_allocator& render_graph_allocator,
    vren::light_array const& light_array,
    vren::vk_utils::buffer const& bvh_buffer,
    size_t bvh_buffer_offset,
    vren::vk_utils::buffer const& light_index_buffer,
    size_t light_index_buffer_offset
)
{
    uint32_t light_count = light_array.m_point_light_count; // TODO: at the moment only point lights, but also spot lights could be supported

    assert(light_count > 0);
    assert(bvh_buffer.m_allocation_info.size >= get_required_bvh_buffer_size(light_count));
    assert(light_index_buffer.m_allocation_info.size >= get_required_light_index_buffer_size(light_count));

    vren::render_graph_node* node = render_graph_allocator.allocate();

    node->set_src_stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    node->set_dst_stage(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    node->add_buffer({ .m_name = "bvh_buffer", .m_buffer = bvh_buffer.m_buffer.m_handle }, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
    node->add_buffer({ .m_name = "light_index_buffer", .m_buffer = light_index_buffer.m_buffer.m_handle }, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
    node->add_buffer({ .m_name = "point_light_position_buffer", .m_buffer = light_array.m_point_light_position_buffer.m_buffer.m_handle }, VK_ACCESS_MEMORY_READ_BIT);
    node->add_buffer({ .m_name = "point_light_buffer", .m_buffer = light_array.m_point_light_buffer.m_buffer.m_handle }, VK_ACCESS_MEMORY_READ_BIT);

    node->set_callback([this, light_count, &light_array, &bvh_buffer, &light_index_buffer](
        uint32_t frame_idx,
        VkCommandBuffer command_buffer,
        vren::resource_container& resource_container
    )
    {
        uint32_t light_count_power_of_2 = vren::round_to_next_power_of_2(light_count); // For reduction operations
        uint32_t light_count_power_of_32 = vren::round_to_next_power_of(light_count, 32u); // For BVH construction

        size_t bvh_size = vren::calc_bvh_buffer_size(light_count);

        VkBufferMemoryBarrier buffer_memory_barrier{};
        uint32_t num_workgroups{};
        std::shared_ptr<vren::pooled_vk_descriptor_set> descriptor_set{};
        std::array<VkBufferCopy, 3> buffer_copies{};

        vren::vk_utils::buffer const& point_light_buffer = light_array.m_point_light_buffer;
        vren::vk_utils::buffer const& light_position_buffer = light_array.m_point_light_position_buffer;
        vren::vk_utils::buffer const& scratch_buffer_1 = bvh_buffer;
        vren::vk_utils::buffer const& scratch_buffer_2 = light_index_buffer;

        // Reduce the light position buffer to scratch_buffer_1 in order to find max

        m_context->m_toolbox->m_reduce_vec4_max(
            command_buffer,
            resource_container,
            light_position_buffer,
            light_count,
            0,
            scratch_buffer_1,
            VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 2,
            1
        );

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 2,
            .size = light_count_power_of_2 * sizeof(glm::vec4)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        // Reduce the light_position_buffer to scratch_buffer_1 in order to find min

        m_context->m_toolbox->m_reduce_vec4_min(
            command_buffer,
            resource_container,
            light_position_buffer,
            light_count,
            0,
            scratch_buffer_1,
            VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT,
            1
        );

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT,
            .size = light_count_power_of_2 * sizeof(glm::vec4)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        // Copy min/max next to each other

        size_t min_max_offset = VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 3 + vren::round_to_next_multiple_of(light_count * sizeof(glm::vec4), VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT);

        buffer_copies = {
            VkBufferCopy{ // min
                .srcOffset = VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT + (light_count_power_of_2 - 1) * sizeof(glm::vec4),
                .dstOffset = min_max_offset,
                .size = sizeof(glm::vec4)
            },
            VkBufferCopy{ // max
                .srcOffset = VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 2 + (light_count_power_of_2 - 1) * sizeof(glm::vec4),
                .dstOffset = min_max_offset + sizeof(glm::vec4),
                .size = sizeof(glm::vec4)
            }
        };
        vkCmdCopyBuffer(command_buffer, scratch_buffer_1.m_buffer.m_handle, scratch_buffer_1.m_buffer.m_handle, 2, buffer_copies.data());

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = min_max_offset,
            .size = 2 * sizeof(glm::vec4)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        // Discretize the light_position_buffer using min/max to obtain morton codes

        m_discretize_light_positions_pipeline.bind(command_buffer);

        descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_discretize_light_positions_pipeline.m_descriptor_set_layouts.at(0))
        );

        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, scratch_buffer_1.m_buffer.m_handle, 2 * sizeof(glm::vec4), min_max_offset);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, light_position_buffer.m_buffer.m_handle, light_count * sizeof(glm::vec4), 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 2, scratch_buffer_1.m_buffer.m_handle, light_count * sizeof(glm::uvec2), 0);

        resource_container.add_resource(descriptor_set);

        m_discretize_light_positions_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        num_workgroups = vren::divide_and_ceil(light_count, 1024);
        m_discretize_light_positions_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

        // Sort morton codes using bucket_sort to improve locality before BVH construction, the sorted morton codes are stored in scratch_buffer_2

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = light_count * sizeof(glm::uvec2)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        m_context->m_toolbox->m_bucket_sort(
            command_buffer,
            resource_container,
            scratch_buffer_1,
            light_count,
            0,
            scratch_buffer_2,
            0
        );

        // Transform sorted morton codes to BVH leaves on scratch_buffer_1, after that we delegate BVH construction to the build_bvh primitive
       
        buffer_memory_barrier = { // Wait for sorted morton codes to be written before reading them
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_2.m_buffer.m_handle,
            .offset = 0,
            .size = light_count * sizeof(glm::uvec2)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        buffer_memory_barrier = { // Also wait for scratch_buffer_1 to be free as it will be used to store the BVH
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE//glm::max(light_count * sizeof(glm::uvec2), bvh_size)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        m_init_light_array_bvh_pipeline.bind(command_buffer);

        descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_init_light_array_bvh_pipeline.m_descriptor_set_layouts.at(0))
        );

        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, point_light_buffer.m_buffer.m_handle, light_count * sizeof(vren::point_light), 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, light_position_buffer.m_buffer.m_handle, light_count * sizeof(glm::vec4), 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 2, scratch_buffer_2.m_buffer.m_handle, light_count * sizeof(glm::uvec2), 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 3, scratch_buffer_1.m_buffer.m_handle, light_count_power_of_32 * sizeof(vren::bvh_node), 0);

        resource_container.add_resource(descriptor_set);

        m_init_light_array_bvh_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

        num_workgroups = vren::divide_and_ceil(light_count_power_of_32, 1024);
        m_init_light_array_bvh_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

        // Finally build the BVH (using build_bvh primitive)

        buffer_memory_barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = scratch_buffer_1.m_buffer.m_handle,
            .offset = 0,
            .size = bvh_size
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        m_context->m_toolbox->m_build_bvh(command_buffer, resource_container, scratch_buffer_1, light_count_power_of_32, nullptr);
    });

    return vren::render_graph_gather(node);
}
