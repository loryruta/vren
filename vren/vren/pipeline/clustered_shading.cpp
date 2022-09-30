#include "clustered_shading.hpp"

#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"

vren::clustered_shading::construct_point_light_bvh::construct_point_light_bvh(vren::context const& context) :
    m_context(&context),
    m_point_light_position_to_view_space_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/point_light_position_to_view_space.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }()),
    m_discretize_point_light_positions_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/discretize_point_light_positions.comp.spv");
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

VkBufferUsageFlags vren::clustered_shading::construct_point_light_bvh::get_required_bvh_buffer_usage_flags()
{
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
}

size_t vren::clustered_shading::construct_point_light_bvh::get_required_bvh_buffer_size(uint32_t point_light_count) // scratch_buffer_1
{
    uint32_t light_count_power_of_2 = vren::round_to_next_power_of_2(point_light_count);

    return glm::max<size_t>(
        glm::max<size_t>(
            vren::build_bvh::get_required_buffer_size(point_light_count), // BVH
            point_light_count * sizeof(glm::uvec2) // Morton codes
        ),
        VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 3 + vren::round_to_next_multiple_of(light_count_power_of_2 * sizeof(glm::vec4), VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT) + 2 * sizeof(glm::vec4) // Reduction + min/max
    );
}

VkBufferUsageFlags vren::clustered_shading::construct_point_light_bvh::get_required_point_light_index_buffer_usage_flags()
{
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | vren::bucket_sort::get_required_output_buffer_usage_flags();
}

size_t vren::clustered_shading::construct_point_light_bvh::get_required_point_light_index_buffer_size(uint32_t light_count) // scratch_buffer_2
{
    return glm::max<size_t>(
        light_count * sizeof(glm::uvec2),
        (light_count + 2) * sizeof(glm::vec4)
    );
}

void vren::clustered_shading::construct_point_light_bvh::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::light_array const& light_array,
    vren::vk_utils::buffer const& view_space_point_light_position_buffer,
    vren::camera const& camera,
    vren::vk_utils::buffer const& bvh_buffer,
    vren::vk_utils::buffer const& point_light_index_buffer
)
{
    uint32_t point_light_count = light_array.m_point_light_count;

    assert(bvh_buffer.m_allocation_info.size >= get_required_bvh_buffer_size(point_light_count));
    assert(point_light_index_buffer.m_allocation_info.size >= get_required_point_light_index_buffer_size(point_light_count));

    uint32_t light_count_power_of_2 = vren::round_to_next_power_of_2(point_light_count); // For reduction operations
    uint32_t light_count_for_bvh = vren::calc_bvh_padded_leaf_count(point_light_count); // For BVH construction

    size_t bvh_size = vren::calc_bvh_buffer_size(point_light_count);

    VkBufferMemoryBarrier buffer_memory_barrier{};
    uint32_t num_workgroups{};
    std::shared_ptr<vren::pooled_vk_descriptor_set> descriptor_set{};
    std::array<VkBufferCopy, 3> buffer_copies{};

    vren::vk_utils::buffer const& point_light_buffer = light_array.m_point_light_buffer;
    vren::vk_utils::buffer const& scratch_buffer_1 = bvh_buffer;
    vren::vk_utils::buffer const& scratch_buffer_2 = point_light_index_buffer;

    // ------------------------------------------------------------------------------------------------
    // 1. Convert the world-space point light positions to view-space
    // ------------------------------------------------------------------------------------------------

    // Bind pipeline
    m_point_light_position_to_view_space_pipeline.bind(command_buffer);

    // Push constants
    {
        struct {
            glm::mat4 m_camera_view;
        } push_constants;

        push_constants = {
            .m_camera_view = camera.get_view(),
        };

        m_point_light_position_to_view_space_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);
    }

    // Descriptor set 0
    descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_point_light_position_to_view_space_pipeline.m_descriptor_set_layouts.at(0))
        );

    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, light_array.m_point_light_position_buffer.m_buffer.m_handle, point_light_count * sizeof(glm::vec4), 0);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, view_space_point_light_position_buffer.m_buffer.m_handle, point_light_count * sizeof(glm::vec4), 0);

    m_point_light_position_to_view_space_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    // Dispatch
    m_point_light_position_to_view_space_pipeline.dispatch(command_buffer, vren::divide_and_ceil(point_light_count, 1024), 1, 1);

    resource_container.add_resource(descriptor_set);

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = view_space_point_light_position_buffer.m_buffer.m_handle,
        .offset = 0,
        .size = point_light_count * sizeof(glm::vec4)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    // ------------------------------------------------------------------------------------------------
    // 2. Reduce the light positions to find max
    // ------------------------------------------------------------------------------------------------

    m_context->m_toolbox->m_reduce_vec4_max(
        command_buffer,
        resource_container,
        view_space_point_light_position_buffer,
        point_light_count,
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

    // ------------------------------------------------------------------------------------------------
    // 3. Reduce the light positions to find min
    // ------------------------------------------------------------------------------------------------

    m_context->m_toolbox->m_reduce_vec4_min(
        command_buffer,
        resource_container,
        view_space_point_light_position_buffer,
        point_light_count,
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

    // ------------------------------------------------------------------------------------------------
    // 4. Copy min/max next to each other
    // ------------------------------------------------------------------------------------------------

    size_t min_max_offset = VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT * 3 + vren::round_to_next_multiple_of(point_light_count * sizeof(glm::vec4), VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT);

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

    // ------------------------------------------------------------------------------------------------
    // 5. Discretize light positions using min/max to obtain morton codes
    // ------------------------------------------------------------------------------------------------

    m_discretize_point_light_positions_pipeline.bind(command_buffer);

    descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_discretize_point_light_positions_pipeline.m_descriptor_set_layouts.at(0))
        );

    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, scratch_buffer_1.m_buffer.m_handle, 2 * sizeof(glm::vec4), min_max_offset);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, view_space_point_light_position_buffer.m_buffer.m_handle, point_light_count * sizeof(glm::vec4), 0);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 2, scratch_buffer_1.m_buffer.m_handle, point_light_count * sizeof(glm::uvec2), 0);


    m_discretize_point_light_positions_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    num_workgroups = vren::divide_and_ceil(point_light_count, 1024);
    m_discretize_point_light_positions_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

    resource_container.add_resource(descriptor_set);

    // ------------------------------------------------------------------------------------------------
    // 6. Sort the morton codes using BucketSort to improve locality before BVH construction
    // ------------------------------------------------------------------------------------------------

    buffer_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = scratch_buffer_1.m_buffer.m_handle,
        .offset = 0,
        .size = point_light_count * sizeof(glm::uvec2)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    m_context->m_toolbox->m_bucket_sort(
        command_buffer,
        resource_container,
        scratch_buffer_1,
        point_light_count,
        0,
        scratch_buffer_2,
        0
    );

    // ------------------------------------------------------------------------------------------------
    // 7. Transform sorted morton codes in BVH leaves, after that we delegate BVH construction to the BuildBVH primitive
    // ------------------------------------------------------------------------------------------------

    buffer_memory_barrier = { // Wait for sorted morton codes to be written before reading them
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = scratch_buffer_2.m_buffer.m_handle,
        .offset = 0,
        .size = point_light_count * sizeof(glm::uvec2)
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

    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, point_light_buffer.m_buffer.m_handle, point_light_count * sizeof(vren::point_light), 0);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, view_space_point_light_position_buffer.m_buffer.m_handle, point_light_count * sizeof(glm::vec4), 0);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 2, scratch_buffer_2.m_buffer.m_handle, point_light_count * sizeof(glm::uvec2), 0);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 3, scratch_buffer_1.m_buffer.m_handle, light_count_for_bvh * sizeof(vren::bvh_node), 0);

    resource_container.add_resource(descriptor_set);

    m_init_light_array_bvh_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    num_workgroups = vren::divide_and_ceil(light_count_for_bvh, 1024);
    m_init_light_array_bvh_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

    // ------------------------------------------------------------------------------------------------
    // 8. Finally build the BVH
    // ------------------------------------------------------------------------------------------------

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

    m_context->m_toolbox->m_build_bvh(
        command_buffer,
        resource_container,
        scratch_buffer_1,
        light_count_for_bvh
    );
}


// --------------------------------------------------------------------------------------------------------------------------------
// find_unique_cluster_list
// --------------------------------------------------------------------------------------------------------------------------------

vren::clustered_shading::find_unique_cluster_list::find_unique_cluster_list(vren::context const& context) :
    m_context(&context),
    m_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/find_unique_clusters.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::clustered_shading::find_unique_cluster_list::operator()(
    uint32_t frame_idx,
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    glm::uvec2 const& screen,
    vren::camera const& camera,
    vren::gbuffer const& gbuffer,
    vren::vk_utils::depth_buffer_t const& depth_buffer,
    vren::vk_utils::buffer const& cluster_key_buffer,
    vren::vk_utils::buffer const& allocation_index_buffer,
    vren::vk_utils::combined_image_view const& cluster_reference_buffer
)
{
    assert(gbuffer.m_width == screen.x);
    assert(gbuffer.m_height == screen.y);

    m_pipeline.bind(command_buffer);

    glm::uvec4 allocation_index_init{ 1, 1, 1, 0 };
    vkCmdUpdateBuffer(command_buffer, allocation_index_buffer.m_buffer.m_handle, 0, sizeof(allocation_index_init), &allocation_index_init);

    VkBufferMemoryBarrier buffer_memory_barrier{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = allocation_index_buffer.m_buffer.m_handle,
        .offset = 0,
        .size = sizeof(glm::uvec4)
    };
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

    struct
    {
        float m_camera_near;
        float m_camera_half_fov_y;
        float _pad[2];
        glm::mat4 m_camera_projection;
    } push_constants;

    push_constants = {
        .m_camera_near = camera.m_near_plane,
        .m_camera_half_fov_y = camera.m_fov_y / 2.0f,
        .m_camera_projection = camera.get_projection(),
    };
    m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

    auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
    );
    gbuffer.write_descriptor_set(descriptor_set_0->m_handle.m_descriptor_set, depth_buffer);

    m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);

    auto descriptor_set_1 = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(1))
    );
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_1->m_handle.m_descriptor_set, 0, cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_1->m_handle.m_descriptor_set, 1, allocation_index_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
    vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_1->m_handle.m_descriptor_set, 2, cluster_reference_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);

    m_pipeline.bind_descriptor_set(command_buffer, 1, descriptor_set_1->m_handle.m_descriptor_set);

    uint32_t num_workgroups_x = vren::divide_and_ceil(screen.x, 32);
    uint32_t num_workgroups_y = vren::divide_and_ceil(screen.y, 32);
    m_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);

    resource_container.add_resources(
        descriptor_set_0,
        descriptor_set_1
    );
}

// --------------------------------------------------------------------------------------------------------------------------------
// assign_lights
// --------------------------------------------------------------------------------------------------------------------------------

vren::clustered_shading::assign_lights::assign_lights(
    vren::context const& context
) :
    m_context(&context),
    m_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/assign_lights.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::clustered_shading::assign_lights::operator()(
    uint32_t frame_idx,
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    glm::uvec2 const& screen,
    vren::camera const& camera,
    vren::vk_utils::buffer const& cluster_key_buffer,
    vren::vk_utils::buffer const& allocation_index_buffer,
    vren::vk_utils::buffer const& light_bvh_buffer,
    uint32_t light_bvh_root_index,
    uint32_t light_count,
    vren::vk_utils::buffer const& light_index_buffer,
    vren::vk_utils::buffer const& assigned_light_buffer,
    vren::vk_utils::buffer const& view_space_point_light_position_buffer
)
{
    vkCmdFillBuffer(command_buffer, assigned_light_buffer.m_buffer.m_handle, 0, VK_WHOLE_SIZE, UINT32_MAX);

    if (light_count > 0)
    {
        VkBufferMemoryBarrier buffer_memory_barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = assigned_light_buffer.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        // Bind pipeline
        m_pipeline.bind(command_buffer);

        // Push constants
        struct
        {
            glm::uvec2 m_num_tiles;
            float m_camera_near;
            float m_camera_half_fov_y;
            glm::mat4 m_camera_proj;
            glm::mat4 m_camera_view;
            glm::uint m_bvh_root_index;
            float _pad1[3];
        } push_constants;

        push_constants = {
            .m_num_tiles = glm::uvec2(
                vren::divide_and_ceil(screen.x, 32u),
                vren::divide_and_ceil(screen.y, 32u)
            ),
            .m_camera_near = camera.m_near_plane,
            .m_camera_half_fov_y = camera.m_fov_y / 2.0f,
            .m_camera_proj = camera.get_projection(),
            .m_camera_view = camera.get_view(),
            .m_bvh_root_index = light_bvh_root_index,
        };

        m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        // Bind descriptor set 0
        auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
        );
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 0, cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 1, allocation_index_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 2, light_bvh_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 3, light_index_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 4, assigned_light_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);

        m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);

        // Bind descriptor set 1
        auto descriptor_set_1 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(1))
        );
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_1->m_handle.m_descriptor_set, 0, view_space_point_light_position_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);

        m_pipeline.bind_descriptor_set(command_buffer, 1, descriptor_set_1->m_handle.m_descriptor_set);

        // Dispatch
        vkCmdDispatchIndirect(command_buffer, allocation_index_buffer.m_buffer.m_handle, 0);

        // 
        resource_container.add_resources(
            descriptor_set_0,
            descriptor_set_1
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------------------
// shade
// --------------------------------------------------------------------------------------------------------------------------------

vren::clustered_shading::shade::shade(
    vren::context const& context
) :
    m_context(&context),
    m_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/clustered_shading/shade.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::clustered_shading::shade::operator()(
    uint32_t frame_idx,
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    glm::uvec2 const& screen,
    vren::camera const& camera,
    vren::gbuffer const& gbuffer,
    vren::vk_utils::depth_buffer_t const& depth_buffer,
    vren::vk_utils::combined_image_view const& cluster_reference_buffer,
    vren::vk_utils::buffer const& assigned_light_buffer,
    vren::light_array const& light_array,
    vren::material_buffer const& material_buffer,
    vren::vk_utils::combined_image_view const& output
)
{
    assert(gbuffer.m_width == screen.x);
    assert(gbuffer.m_height == screen.y);

    m_pipeline.bind(command_buffer);

    // Push constants
    struct
    {
        glm::vec3 m_camera_position; float m_camera_far_plane;
        glm::mat4 m_camera_view;
        glm::mat4 m_camera_projection;
    } push_constants;

    push_constants = {
        .m_camera_position = camera.m_position,
        .m_camera_far_plane = camera.m_far_plane,
        .m_camera_view = camera.get_view(),
        .m_camera_projection = camera.get_projection(),
    };

    m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

    // Descriptors
    {
        // gBuffer
        auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
        );
        gbuffer.write_descriptor_set(descriptor_set_0->m_handle.m_descriptor_set, depth_buffer);

        m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);

        // Clustered shading buffers
        auto descriptor_set_1 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(1))
        );

        vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_1->m_handle.m_descriptor_set, 0, cluster_reference_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_1->m_handle.m_descriptor_set, 1, assigned_light_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);

        m_pipeline.bind_descriptor_set(command_buffer, 1, descriptor_set_1->m_handle.m_descriptor_set);

        // Textures
        auto descriptor_set_2 = m_context->m_toolbox->m_texture_manager.m_descriptor_set;
        m_pipeline.bind_descriptor_set(command_buffer, 2, descriptor_set_2->m_handle.m_descriptor_set);

        // Light array
        auto descriptor_set_3 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(3))
        );
        light_array.write_descriptor_set(descriptor_set_3->m_handle.m_descriptor_set);

        m_pipeline.bind_descriptor_set(command_buffer, 3, descriptor_set_3->m_handle.m_descriptor_set);

        // Materials
        auto descriptor_set_4 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(4))
        );
        material_buffer.write_descriptor_set(descriptor_set_4->m_handle.m_descriptor_set);

        m_pipeline.bind_descriptor_set(command_buffer, 4, descriptor_set_4->m_handle.m_descriptor_set);

        // Output
        auto descriptor_set_5 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(5))
        );
        vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_5->m_handle.m_descriptor_set, 0, output.get_image_view(), VK_IMAGE_LAYOUT_GENERAL);

        m_pipeline.bind_descriptor_set(command_buffer, 5, descriptor_set_5->m_handle.m_descriptor_set);

        resource_container.add_resources(
            descriptor_set_0,
            descriptor_set_1,
            descriptor_set_2,
            descriptor_set_3,
            descriptor_set_4,
            descriptor_set_5
        );
    }

    uint32_t num_workgroups_x = vren::divide_and_ceil(screen.x, 32);
    uint32_t num_workgroups_y = vren::divide_and_ceil(screen.y, 32);
    m_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);
}

// --------------------------------------------------------------------------------------------------------------------------------
// cluster_and_shade
// --------------------------------------------------------------------------------------------------------------------------------

vren::cluster_and_shade::cluster_and_shade(
    vren::context const& context
) :
    m_context(&context),

    m_construct_point_light_bvh(context),
    m_find_unique_cluster_list(context),
    m_assign_lights(context),
    m_shade(context),

    m_view_space_point_light_position_buffer([&]()
    {
        auto buffer = vren::vk_utils::alloc_device_only_buffer(
            *m_context,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VREN_MAX_POINT_LIGHT_COUNT * sizeof(glm::vec4)
        );
        vren::vk_utils::set_name(*m_context, buffer, "view_space_point_light_position");
        return buffer;
    }()),
    m_point_light_bvh_buffer([&]()
    {
        auto buffer = vren::vk_utils::alloc_device_only_buffer(
            *m_context,
            vren::clustered_shading::construct_point_light_bvh::get_required_bvh_buffer_usage_flags(),
            vren::clustered_shading::construct_point_light_bvh::get_required_bvh_buffer_size(VREN_MAX_POINT_LIGHT_COUNT)
        );
        vren::vk_utils::set_name(*m_context, buffer, "point_light_bvh_buffer");
        return buffer;
    }()),
    m_point_light_index_buffer([&]()
    {
        auto buffer = vren::vk_utils::alloc_device_only_buffer(
            *m_context,
            vren::clustered_shading::construct_point_light_bvh::get_required_point_light_index_buffer_usage_flags(),
            vren::clustered_shading::construct_point_light_bvh::get_required_point_light_index_buffer_size(VREN_MAX_POINT_LIGHT_COUNT)
        );
        vren::vk_utils::set_name(*m_context, buffer, "point_light_index_buffer");
        return buffer;
    }()),

    m_cluster_key_buffer(vren::vk_utils::alloc_device_only_buffer(
        *m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VREN_MAX_UNIQUE_CLUSTER_KEYS * sizeof(uint32_t)
    )),
    m_allocation_index_buffer(vren::vk_utils::alloc_device_only_buffer(
        *m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        sizeof(glm::uvec4)
    )),
    m_cluster_reference_buffer([&]()
    {
        vren::vk_utils::image image = vren::vk_utils::create_image(*m_context, VREN_MAX_SCREEN_WIDTH, VREN_MAX_SCREEN_HEIGHT, VK_FORMAT_R32_UINT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_STORAGE_BIT);
        vren::vk_image_view image_view = vren::vk_utils::create_image_view(*m_context, image.m_image.m_handle, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT);

        return vren::vk_utils::combined_image_view{
            .m_image = std::move(image),
            .m_image_view = std::move(image_view)
        };
    }()),
    m_assigned_light_buffer(vren::vk_utils::alloc_device_only_buffer(
        *m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VREN_MAX_UNIQUE_CLUSTER_KEYS * VREN_MAX_LIGHTS_PER_CLUSTER * sizeof(uint32_t)
    ))
{
    vren::vk_utils::set_name(*m_context, m_cluster_key_buffer, "cluster_key_buffer");
    vren::vk_utils::set_name(*m_context, m_allocation_index_buffer, "allocation_index_buffer");
    vren::vk_utils::set_name(*m_context, m_cluster_reference_buffer, "cluster_reference_buffer");
    vren::vk_utils::set_name(*m_context, m_assigned_light_buffer, "assigned_light_buffer");

    vren::vk_utils::immediate_graphics_queue_submit(*m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        VkImageMemoryBarrier image_memory_barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = NULL,
            .dstAccessMask = NULL,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_cluster_reference_buffer.get_image(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
    });
}

vren::render_graph_t vren::cluster_and_shade::operator()(
    vren::render_graph_allocator& allocator,
    glm::uvec2 const& screen,
    vren::camera const& camera,
    vren::gbuffer const& gbuffer,
    vren::vk_utils::depth_buffer_t const& depth_buffer,
    vren::light_array const& light_array,
    vren::material_buffer const& material_buffer,
    vren::vk_utils::combined_image_view const& output
)
{
    vren::render_graph_node* node = allocator.allocate();

    node->set_name("cluster_and_shade");

    node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    gbuffer.add_render_graph_node_resources(*node, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
    node->add_image({
        .m_image = depth_buffer.get_image(),
        .m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        .m_mip_level = 0,
        .m_layer = 0,
    }, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
    light_array.add_render_graph_node_resources(*node, VK_ACCESS_SHADER_READ_BIT);
    node->add_image({
        .m_image = output.get_image(),
        .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .m_mip_level = 0,
        .m_layer = 0,
    }, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);

    node->set_callback([
        this,
        camera,
        screen,
        &gbuffer,
        &depth_buffer,
        &light_array,
        &material_buffer,
        &output
    ](
        uint32_t frame_idx,
        VkCommandBuffer command_buffer,
        vren::resource_container& resource_container
    )
    {
        std::array<VkBufferMemoryBarrier, 3> buffer_memory_barriers{};
        std::array<VkImageMemoryBarrier, 3> image_memory_barriers{};

        // ------------------------------------------------------------------------------------------------
        // 1. Construct point light BVH
        // ------------------------------------------------------------------------------------------------

        if (light_array.m_point_light_count > 0)
        {
            m_construct_point_light_bvh(
                command_buffer,
                resource_container,
                light_array,
                m_view_space_point_light_position_buffer,
                camera,
                m_point_light_bvh_buffer,
                m_point_light_index_buffer
            );

            buffer_memory_barriers = {
                VkBufferMemoryBarrier{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = m_point_light_bvh_buffer.m_buffer.m_handle,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE
                },
                VkBufferMemoryBarrier{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = m_point_light_index_buffer.m_buffer.m_handle,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE
                },
            };
            vkCmdPipelineBarrier(
                command_buffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                NULL,
                0, nullptr,
                2, buffer_memory_barriers.data(),
                0, nullptr
            );
        }

        // ------------------------------------------------------------------------------------------------
        // 2. Find unique cluster list
        // ------------------------------------------------------------------------------------------------

        m_find_unique_cluster_list(
            frame_idx,
            command_buffer,
            resource_container,
            screen,
            camera,
            gbuffer,
            depth_buffer,
            m_cluster_key_buffer,
            m_allocation_index_buffer,
            m_cluster_reference_buffer
        );

        buffer_memory_barriers = {
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_cluster_key_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_allocation_index_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
        };
        image_memory_barriers = {
            VkImageMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = m_cluster_reference_buffer.get_image(),
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            }
        };
        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            NULL,
            0, nullptr,
            2, buffer_memory_barriers.data(),
            1, image_memory_barriers.data()
        );

        // ------------------------------------------------------------------------------------------------
        // 3. Lights assignment
        // ------------------------------------------------------------------------------------------------

        m_assign_lights(
            frame_idx,
            command_buffer,
            resource_container,
            screen,
            camera,
            m_cluster_key_buffer,
            m_allocation_index_buffer,
            m_point_light_bvh_buffer,
            vren::calc_bvh_root_index(light_array.m_point_light_count),
            light_array.m_point_light_count,
            m_point_light_index_buffer,
            m_assigned_light_buffer,
            m_view_space_point_light_position_buffer
        );

        buffer_memory_barriers = {
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_assigned_light_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, buffer_memory_barriers.data(), 0, nullptr);

        // ------------------------------------------------------------------------------------------------
        // 4. Shading
        // ------------------------------------------------------------------------------------------------

        m_shade(
            frame_idx,
            command_buffer,
            resource_container,
            screen,
            camera,
            gbuffer,
            depth_buffer,
            m_cluster_reference_buffer,
            m_assigned_light_buffer,
            light_array,
            material_buffer,
            output
        );
    });

    return vren::render_graph_gather(node);
}
