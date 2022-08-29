#include "clustered_shading.hpp"

#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"

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
    vren::vk_utils::buffer const& assigned_light_buffer
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
            .buffer = assigned_light_buffer.m_buffer.m_handle,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        m_pipeline.bind(command_buffer);

        struct
        {
            float m_camera_near;
            float m_camera_half_fov_y;
            float m_camera_aspect_ratio;
            glm::mat4 m_camera_view;
            glm::uint m_bvh_root_index;
            glm::uvec2 m_num_tiles;
        } push_constants;

        push_constants = {
            .m_camera_near = camera.m_near_plane,
            .m_camera_half_fov_y = camera.m_fov_y / 2.0f,
            .m_camera_aspect_ratio = camera.m_aspect_ratio,
            .m_camera_view = camera.get_view(),
            .m_bvh_root_index = light_bvh_root_index,
            .m_num_tiles = screen / glm::uvec2(32)
        };

        m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

        auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
            m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
            );
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 0, cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 1, allocation_index_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 2, light_bvh_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 3, light_index_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 4, assigned_light_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);

        m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);

        vkCmdDispatchIndirect(command_buffer, allocation_index_buffer.m_buffer.m_handle, 0);

        resource_container.add_resources(
            descriptor_set_0
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
// clusterr_and_shade
// --------------------------------------------------------------------------------------------------------------------------------

vren::cluster_and_shade::cluster_and_shade(
    vren::context const& context
) :
    m_context(&context),

    m_find_unique_cluster_list(context),
    m_assign_lights(context),
    m_shade(context),

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
        VREN_MAX_UNIQUE_CLUSTER_KEYS * VREN_MAX_ASSIGNED_LIGHTS_PER_CLUSTER * sizeof(uint32_t)
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
    vren::vk_utils::buffer const& light_bvh_buffer,
    uint32_t light_bvh_root_index,
    uint32_t light_count,
    vren::vk_utils::buffer const& light_index_buffer,
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
    node->add_buffer({ .m_buffer = light_bvh_buffer.m_buffer.m_handle, }, VK_ACCESS_SHADER_READ_BIT);
    node->add_buffer({ .m_buffer = light_index_buffer.m_buffer.m_handle, }, VK_ACCESS_SHADER_READ_BIT);
    light_array.add_render_graph_node_resources(*node, VK_ACCESS_SHADER_READ_BIT);
    node->add_image({ .m_image = output.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = 0, .m_layer = 0, }, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);

    node->set_callback([
        this,
        camera,
        screen,
        &gbuffer,
        &depth_buffer,
        &light_bvh_buffer,
        light_bvh_root_index,
        light_count,
        &light_index_buffer,
        &light_array,
        &material_buffer,
        &output
    ](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        std::array<VkBufferMemoryBarrier, 3> buffer_memory_barriers{};
        std::array<VkImageMemoryBarrier, 3> image_memory_barriers{};

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
                .buffer = m_cluster_key_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
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
                .image = m_cluster_reference_buffer.get_image(),
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
            }
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 2, buffer_memory_barriers.data(), 1, image_memory_barriers.data());

        m_assign_lights(
            frame_idx,
            command_buffer,
            resource_container,
            screen,
            camera,
            m_cluster_key_buffer,
            m_allocation_index_buffer,
            light_bvh_buffer,
            light_bvh_root_index,
            light_count,
            light_index_buffer,
            m_assigned_light_buffer
        );

        buffer_memory_barriers = {
            VkBufferMemoryBarrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .buffer = m_assigned_light_buffer.m_buffer.m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, buffer_memory_barriers.data(), 0, nullptr);

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
