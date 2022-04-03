#include "toolbox.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Toolbox
// --------------------------------------------------------------------------------------------------------------------------------

vren::command_pool vren::toolbox::create_graphics_command_pool(vren::context const& ctx)
{
	VkCommandPoolCreateInfo cmd_pool_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx.m_queue_families.m_graphics_idx,
	};

	VkCommandPool cmd_pool;
	vren::vk_utils::check(vkCreateCommandPool(ctx.m_device, &cmd_pool_info, nullptr, &cmd_pool));
	return vren::command_pool(ctx, vren::vk_command_pool(ctx, cmd_pool));
}

vren::command_pool vren::toolbox::create_transfer_command_pool(vren::context const& ctx)
{
	VkCommandPoolCreateInfo cmd_pool_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex =  ctx.m_queue_families.m_transfer_idx,
	};

	VkCommandPool cmd_pool;
	vren::vk_utils::check(vkCreateCommandPool(ctx.m_device, &cmd_pool_info, nullptr, &cmd_pool));
	return vren::command_pool(ctx, vren::vk_command_pool(ctx, cmd_pool));
}

vren::toolbox::toolbox(vren::context const& ctx) :
	m_context(&ctx),

	m_graphics_command_pool(create_graphics_command_pool(ctx)),
	m_transfer_command_pool(create_transfer_command_pool(ctx)),
	m_fence_pool(ctx),
	m_descriptor_pool(ctx),

	m_white_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(ctx, 255, 255, 255, 255))),
	m_black_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(ctx, 0, 0, 0, 255))),
	m_red_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(ctx, 255, 0, 0, 255))),
	m_green_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(ctx, 0, 255, 0, 255))),
	m_blue_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(ctx, 0, 0, 255, 255)))
{}
