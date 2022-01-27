#include "command_pool.hpp"

#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Command pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::command_pool::command_pool(
	std::shared_ptr<vren::context> const& ctx,
	VkCommandPool handle
) :
	m_context(ctx),
	m_handle(handle)
{}

vren::command_pool::~command_pool()
{
	vkDestroyCommandPool(m_context->m_device, m_handle, nullptr);
}

vren::vk_command_buffer vren::command_pool::acquire_command_buffer()
{
	VkCommandBuffer cmd_buf;

	if (m_initial_state_cmd_buffers.empty())
	{
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.pNext = nullptr;
		alloc_info.commandPool = m_handle;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		vren::vk_utils::check(vkAllocateCommandBuffers(m_context->m_device, &alloc_info, &cmd_buf));
	}
	else
	{
		cmd_buf = m_initial_state_cmd_buffers.back();
		m_initial_state_cmd_buffers.pop_back();
	}

	return vren::vk_command_buffer(shared_from_this(), cmd_buf);
}

void vren::command_pool::_release_command_buffer(vren::vk_command_buffer const& cmd_buf)
{
	vkResetCommandBuffer(cmd_buf.m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	m_initial_state_cmd_buffers.push_back(cmd_buf.m_handle);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Command buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_command_buffer::vk_command_buffer(
	std::shared_ptr<vren::command_pool> const& cmd_pool,
	VkCommandBuffer handle
) :
	m_command_pool(cmd_pool),
	m_handle(handle)
{}

vren::vk_command_buffer::vk_command_buffer(vk_command_buffer&& other)
{
	*this = std::move(other);
}

vren::vk_command_buffer::~vk_command_buffer()
{
	if (m_handle != VK_NULL_HANDLE) {
		m_command_pool->_release_command_buffer(*this);
	}
}

vren::vk_command_buffer& vren::vk_command_buffer::operator=(vk_command_buffer&& other)
{
	std::swap(m_command_pool, other.m_command_pool);
	m_handle = other.m_handle;

	other.m_handle = VK_NULL_HANDLE;

	return *this;
}
