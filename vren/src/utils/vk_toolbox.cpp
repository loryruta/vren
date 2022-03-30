#include "vk_toolbox.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// toolbox
// --------------------------------------------------------------------------------------------------------------------------------

std::shared_ptr<vren::command_pool> vren::vk_utils::toolbox::_create_graphics_command_pool(std::shared_ptr<vren::context> const& ctx)
{
	VkCommandPoolCreateInfo cmd_pool_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = ctx->m_queue_families.m_graphics_idx,
	};

	VkCommandPool cmd_pool;
	vren::vk_utils::check(vkCreateCommandPool(ctx->m_device, &cmd_pool_info, nullptr, &cmd_pool));
	return vren::command_pool::create(ctx, cmd_pool);
}

std::shared_ptr<vren::command_pool> vren::vk_utils::toolbox::_create_transfer_command_pool(std::shared_ptr<vren::context> const& ctx)
{
	VkCommandPoolCreateInfo cmd_pool_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex =  ctx->m_queue_families.m_transfer_idx,
	};

	VkCommandPool cmd_pool;
	vren::vk_utils::check(vkCreateCommandPool(ctx->m_device, &cmd_pool_info, nullptr, &cmd_pool));
	return vren::command_pool::create(ctx, cmd_pool);
}

vren::vk_utils::toolbox::toolbox(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx),

	m_graphics_command_pool(_create_graphics_command_pool(ctx)),
	m_transfer_command_pool(_create_transfer_command_pool(ctx)),
	m_fence_pool(vren::fence_pool::create(ctx)),

	m_light_array_descriptor_set_layout(std::make_shared<vren::vk_descriptor_set_layout>(vren::create_light_array_descriptor_set_layout(ctx))),
	m_light_array_descriptor_pool(std::make_shared<vren::light_array_descriptor_pool>(ctx, m_light_array_descriptor_set_layout)),

	m_white_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(*this, 255, 255, 255, 255))),
	m_black_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(*this, 0, 0, 0, 255))),
	m_red_texture  (std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(*this, 255, 0, 0, 255))),
	m_green_texture(std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(*this, 0, 255, 0, 255))),
	m_blue_texture (std::make_shared<vren::vk_utils::texture>(vren::vk_utils::create_color_texture(*this, 0, 0, 255, 255)))
{}

std::shared_ptr<vren::vk_utils::toolbox> vren::vk_utils::toolbox::create(std::shared_ptr<vren::context> const& ctx)
{
	return std::shared_ptr<toolbox>(new toolbox(ctx));
}
