#include "depth_buffer_pyramid.hpp"

#include "glm/gtc/integer.hpp"

#include "Toolbox.hpp"
#include "base/base.hpp"
#include "vk_api/debug_utils.hpp"
#include "vk_api/misc_utils.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// depth_buffer_pyramid
// --------------------------------------------------------------------------------------------------------------------------------

vren::depth_buffer_pyramid::depth_buffer_pyramid(vren::context const& context, uint32_t width, uint32_t height) :
    m_context(&context),
    m_base_width(width),
    m_base_height(height),
    m_level_count(glm::log2(glm::max(width, height)) + 1),
    m_image(create_image()),
    m_image_view(create_image_view()),
    m_level_image_views(create_level_image_views()),
    m_sampler(create_sampler())
{
}

vren::vk_utils::utils vren::depth_buffer_pyramid::create_image()
{
    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R32_SFLOAT,
        .extent =
            {
                .width = m_base_width,
                .height = m_base_height,
                .depth = 1,
            },
        .mipLevels = m_level_count,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};

    image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Debug

    VmaAllocationCreateInfo allocation_info{
        .usage = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    VkImage image;
    VmaAllocation allocation;
    VREN_CHECK(
        vmaCreateImage(m_context->m_vma_allocator, &image_info, &allocation_info, &image, &allocation, nullptr),
        m_context
    );

    vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_IMAGE, (uint64_t) image, "depth_buffer_pyramid");

    return {.m_image = vren::vk_image(*m_context, image), .m_allocation = vren::vma_allocation(*m_context, allocation)};
}

vren::vk_image_view vren::depth_buffer_pyramid::create_image_view()
{
    VkImageViewCreateInfo image_view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .image = m_image.m_image.m_handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R32_SFLOAT,
        .components = {},
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = m_level_count,
            .baseArrayLayer = 0,
            .layerCount = 1}};

    VkImageView image_view;
    VREN_CHECK(vkCreateImageView(m_context->m_device, &image_view_info, nullptr, &image_view), m_context);
    return vren::vk_image_view(*m_context, image_view);
}

std::vector<vren::vk_image_view> vren::depth_buffer_pyramid::create_level_image_views()
{
    std::vector<vren::vk_image_view> image_views;

    for (uint32_t i = 0; i < m_level_count; i++)
    {
        VkImageViewCreateInfo image_view_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = NULL,
            .image = m_image.m_image.m_handle,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R32_SFLOAT,
            .components = {},
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = i,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1}};

        VkImageView image_view;
        VREN_CHECK(vkCreateImageView(m_context->m_device, &image_view_info, nullptr, &image_view), m_context);

        vren::vk_utils::set_object_name(
            *m_context,
            VK_OBJECT_TYPE_IMAGE_VIEW,
            (uint64_t) image_view,
            fmt::format("depth_buffer_pyramid level={}", i).c_str()
        );

        image_views.emplace_back(*m_context, image_view);
    }

    return image_views;
}

vren::vk_sampler vren::depth_buffer_pyramid::create_sampler()
{
    VkSamplerReductionModeCreateInfo sampler_reduction_mode_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO,
        .pNext = nullptr,
        .reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX};
    VkSamplerCreateInfo sampler_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = &sampler_reduction_mode_info,
        .flags = NULL,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .minLod = 0.0f,
        .maxLod = (float) m_level_count,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE};
    VkSampler sampler;
    VREN_CHECK(vkCreateSampler(m_context->m_device, &sampler_info, nullptr, &sampler), m_context);
    return vren::vk_sampler(*m_context, sampler);
}

void vren::depth_buffer_pyramid::add_render_graph_node_resources(
    vren::render_graph_node& node, VkImageLayout image_layout, VkAccessFlags access_flags
) const
{
    static const std::array<std::string, 16> k_depth_buffer_pyramid_level_names = vren::create_array<std::string, 16>(
        [](uint32_t index)
        {
            return fmt::format("depth_buffer_pyramid[{}]", index);
        }
    );

    for (uint32_t level = 0; level < m_level_count; level++)
    {
        node.add_image(
            {
                .m_name = k_depth_buffer_pyramid_level_names.at(level).c_str(),
                .m_image = m_image.m_image.m_handle,
                .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
                .m_mip_level = level,
            },
            image_layout,
            access_flags
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------------------
// depth_buffer_reductor
// --------------------------------------------------------------------------------------------------------------------------------

vren::depth_buffer_reductor::depth_buffer_reductor(vren::context const& context) :
    m_context(&context),
    m_copy_pipeline(create_copy_pipeline()),
    m_reduce_pipeline(create_reduce_pipeline()),
    m_depth_buffer_sampler(create_depth_buffer_sampler())
{
}

vren::pipeline vren::depth_buffer_reductor::create_copy_pipeline()
{
    vren::shader_module shader_mod =
        vren::load_shader_module_from_file(*m_context, ".vren/resources/shaders/depth_buffer_copy.comp.spv");
    vren::specialized_shader shader = vren::specialized_shader(shader_mod, "main");

    return vren::create_compute_pipeline(*m_context, shader);
}

vren::pipeline vren::depth_buffer_reductor::create_reduce_pipeline()
{
    vren::shader_module shader_mod =
        vren::load_shader_module_from_file(*m_context, ".vren/resources/shaders/depth_buffer_reduce.comp.spv");
    vren::specialized_shader shader = vren::specialized_shader(shader_mod, "main");

    return vren::create_compute_pipeline(*m_context, shader);
}

vren::vk_sampler vren::depth_buffer_reductor::create_depth_buffer_sampler()
{
    VkSamplerCreateInfo sampler_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .unnormalizedCoordinates = true,
    };
    VkSampler sampler;
    VREN_CHECK(vkCreateSampler(m_context->m_device, &sampler_info, nullptr, &sampler), m_context);
    return vren::vk_sampler(*m_context, sampler);
}

vren::render_graph_t vren::depth_buffer_reductor::copy_and_reduce(
    vren::render_graph_allocator& allocator,
    vren::vk_utils::depth_buffer_t const& depth_buffer,
    vren::depth_buffer_pyramid const& depth_buffer_pyramid
) const
{
    vren::render_graph_node* node = allocator.allocate();

    node->set_name("depth_buffer_pyramid_construction");

    node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    node->add_image(
        {
            .m_name = "depth_buffer",
            .m_image = depth_buffer.get_image(),
            .m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        },
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_ACCESS_SHADER_READ_BIT
    );
    depth_buffer_pyramid.add_render_graph_node_resources(*node, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);

    node->set_callback(
        [this, &depth_buffer, &depth_buffer_pyramid](
            uint32_t frame_idx, VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container
        )
        {
            std::shared_ptr<vren::pooled_vk_descriptor_set> descriptor_set{};
            VkImageMemoryBarrier image_memory_barrier{};

            // Copy depth_buffer to detph_buffer_pyramid's first level
            m_copy_pipeline.bind(command_buffer);

            descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
                m_context->m_toolbox->m_descriptor_pool.acquire(m_copy_pipeline.m_descriptor_set_layouts.at(0))
            );
            vren::vk_utils::write_combined_image_sampler_descriptor(
                *m_context,
                descriptor_set->m_handle.m_descriptor_set,
                0,
                m_depth_buffer_sampler.m_handle,
                depth_buffer.get_image_view(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
            vren::vk_utils::write_storage_image_descriptor(
                *m_context,
                descriptor_set->m_handle.m_descriptor_set,
                1,
                depth_buffer_pyramid.get_level_image_view(0),
                VK_IMAGE_LAYOUT_GENERAL
            );

            m_copy_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

            uint32_t num_workgroups_x = vren::divide_and_ceil(depth_buffer_pyramid.m_base_width, 32);
            uint32_t num_workgroups_y = vren::divide_and_ceil(depth_buffer_pyramid.m_base_height, 32);
            m_copy_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);

            resource_container.add_resource(descriptor_set);

            // Reduce depth_buffer_pyramid levels
            for (uint32_t level = 0; level < depth_buffer_pyramid.get_level_count() - 1; level++)
            {
                image_memory_barrier = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                    .pNext = nullptr,
                    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = depth_buffer_pyramid.get_image(),
                    .subresourceRange =
                        {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel = level,
                            .levelCount = 1,
                            .baseArrayLayer = 0,
                            .layerCount = 1,
                        },
                };
                vkCmdPipelineBarrier(
                    command_buffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    NULL,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &image_memory_barrier
                );

                m_reduce_pipeline.bind(command_buffer);

                descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
                    m_context->m_toolbox->m_descriptor_pool.acquire(m_reduce_pipeline.m_descriptor_set_layouts.at(0))
                );

                vren::vk_utils::write_storage_image_descriptor(
                    *m_context,
                    descriptor_set->m_handle.m_descriptor_set,
                    0,
                    depth_buffer_pyramid.m_level_image_views.at(level).m_handle,
                    VK_IMAGE_LAYOUT_GENERAL
                );
                vren::vk_utils::write_storage_image_descriptor(
                    *m_context,
                    descriptor_set->m_handle.m_descriptor_set,
                    1,
                    depth_buffer_pyramid.m_level_image_views.at(level + 1).m_handle,
                    VK_IMAGE_LAYOUT_GENERAL
                );

                m_reduce_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

                uint32_t num_workgroups_x = vren::divide_and_ceil(depth_buffer_pyramid.get_image_width(level + 1), 32);
                uint32_t num_workgroups_y = vren::divide_and_ceil(depth_buffer_pyramid.get_image_height(level + 1), 32);
                m_reduce_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);

                resource_container.add_resource(descriptor_set);
            }
        }
    );
    return vren::render_graph_gather(node);
}
