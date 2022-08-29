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
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    )),
    m_texcoord_buffer(vren::vk_utils::create_color_buffer(
        context,
        width, height,
        VK_FORMAT_R32G32_SFLOAT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    )),
    m_material_index_buffer(vren::vk_utils::create_color_buffer(
        context,
        width, height,
        VK_FORMAT_R16_UINT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
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