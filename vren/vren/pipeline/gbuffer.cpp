#include "gbuffer.hpp"

vren::gbuffer::gbuffer(vren::context const& context, uint32_t width, uint32_t height) :
    m_context(&context),
    m_width(width),
    m_height(height),
    m_normal_buffer(vren::vk_utils::create_color_buffer(
        context,
        width, height,
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    )),
    m_texcoord_buffer(vren::vk_utils::create_color_buffer(
        context,
        width, height,
        VK_FORMAT_R32G32_SFLOAT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    )),
    m_material_index_buffer(vren::vk_utils::create_color_buffer(
        context,
        width, height,
        VK_FORMAT_R16_UINT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    )),
    m_sampler(vren::vk_utils::create_sampler(
        context,
        VK_FILTER_NEAREST,
        VK_FILTER_NEAREST,
        VK_SAMPLER_MIPMAP_MODE_NEAREST,
        VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT
    ))
{
    vren::vk_utils::set_name(context, m_normal_buffer, "gbuffer_normal");
    vren::vk_utils::set_name(context, m_texcoord_buffer, "gbuffer_texcoord");
    vren::vk_utils::set_name(context, m_material_index_buffer, "gbuffer_material_index");

    vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(m_sampler.m_handle), "gbuffer_sampler");
}

void vren::gbuffer::add_render_graph_node_resources(vren::render_graph_node& node, VkImageLayout image_layout, VkAccessFlags access_flags) const
{
    node.add_image({ .m_name = "gbuffer_normal", .m_image = m_normal_buffer.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT }, image_layout, access_flags);
    node.add_image({ .m_name = "gbuffer_texcoord", .m_image = m_texcoord_buffer.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT }, image_layout, access_flags);
    node.add_image({ .m_name = "gbuffer_material_index", .m_image = m_material_index_buffer.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT }, image_layout, access_flags);
}

void vren::gbuffer::write_descriptor_set(VkDescriptorSet descriptor_set, vren::vk_utils::combined_image_view const& depth_buffer) const
{
    vren::vk_utils::write_combined_image_sampler_descriptor(*m_context, descriptor_set, 0, m_sampler.m_handle, m_normal_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vren::vk_utils::write_combined_image_sampler_descriptor(*m_context, descriptor_set, 1, m_sampler.m_handle, m_texcoord_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vren::vk_utils::write_combined_image_sampler_descriptor(*m_context, descriptor_set, 2, m_sampler.m_handle, m_material_index_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vren::vk_utils::write_combined_image_sampler_descriptor(*m_context, descriptor_set, 3, m_sampler.m_handle, depth_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

vren::render_graph_t vren::clear_gbuffer(
    vren::render_graph_allocator& allocator,
    vren::gbuffer const& gbuffer
)
{
    vren::render_graph_node* node = allocator.allocate();

    node->set_name("clear_gbuffer");

    node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
    node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);

    gbuffer.add_render_graph_node_resources(*node, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);

    node->set_callback([
        &gbuffer
    ](
        uint32_t frame_idx,
        VkCommandBuffer command_buffer,
        vren::resource_container& resource_container
    )
    {
        VkClearColorValue clear_color{};
        VkImageSubresourceRange image_subresource_range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        };

        vkCmdClearColorImage(command_buffer, gbuffer.m_normal_buffer.get_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);
        vkCmdClearColorImage(command_buffer, gbuffer.m_material_index_buffer.get_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);
        vkCmdClearColorImage(command_buffer, gbuffer.m_texcoord_buffer.get_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &image_subresource_range);
    });

    return vren::render_graph_gather(node);
}
