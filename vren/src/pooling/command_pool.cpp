#include "command_pool.hpp"

#include <utility>

#include "context.hpp"
#include "utils/misc.hpp"

vren::command_pool::command_pool(
	vren::context const& ctx,
	vren::vk_command_pool&& cmd_pool
) :
	m_context(&ctx),
	m_command_pool(std::move(cmd_pool))
{}

vren::pooled_vk_command_buffer vren::command_pool::acquire()
{
	auto pooled = try_acquire();
	if (pooled.has_value())
	{
		vkResetCommandBuffer(pooled.value().m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		return std::move(pooled.value());
	}

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = m_command_pool.m_handle;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buf;
    vren::vk_utils::check(vkAllocateCommandBuffers(m_context->m_device, &alloc_info, &cmd_buf));
    return create_managed_object(std::move(cmd_buf));
}

