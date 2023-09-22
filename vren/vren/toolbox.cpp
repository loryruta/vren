#include "toolbox.hpp"

#include "context.hpp"
#include "vk_helpers/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Toolbox
// --------------------------------------------------------------------------------------------------------------------------------

vren::command_pool vren::toolbox::create_graphics_command_pool()
{
    VkCommandPoolCreateInfo cmd_pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_context->m_queue_families.m_graphics_idx,
    };
    VkCommandPool command_pool;
    VREN_CHECK(vkCreateCommandPool(m_context->m_device, &cmd_pool_info, nullptr, &command_pool), m_context);
    return vren::command_pool(*m_context, vren::vk_command_pool(*m_context, command_pool));
}

vren::command_pool vren::toolbox::create_transfer_command_pool()
{
    VkCommandPoolCreateInfo cmd_pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_context->m_queue_families.m_transfer_idx,
    };
    VkCommandPool command_pool;
    VREN_CHECK(vkCreateCommandPool(m_context->m_device, &cmd_pool_info, nullptr, &command_pool), m_context);
    return vren::command_pool(*m_context, vren::vk_command_pool(*m_context, command_pool));
}

vren::descriptor_pool vren::toolbox::create_descriptor_pool()
{
    VkDescriptorPoolSize pool_sizes[]{
        {VK_DESCRIPTOR_TYPE_SAMPLER, 32},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 32},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32}};
    return vren::descriptor_pool(*m_context, 32, pool_sizes);
}

vren::toolbox::toolbox(vren::context const& context) :
    m_context(&context),
    m_graphics_command_pool(create_graphics_command_pool()),
    m_transfer_command_pool(create_transfer_command_pool()),
    m_descriptor_pool(create_descriptor_pool()),
    m_texture_manager(context),

    m_reduce_uint_add(context),
    m_reduce_uint_min(context),
    m_reduce_uint_max(context),
    m_reduce_vec4_add(context),
    m_reduce_vec4_min(context),
    m_reduce_vec4_max(context),

    m_blelloch_scan(context),
    m_radix_sort(context),
    m_bucket_sort(context),

    m_build_bvh(context)
{
}
