#include "command_pool.hpp"

#include <utility>

#include "utils/misc.hpp"

vren::command_pool::command_pool(std::shared_ptr<vren::context> ctx, VkCommandPool handle) :
	m_context(std::move(ctx)),
	m_handle(handle)
{}

vren::command_pool::~command_pool()
{
	vkDestroyCommandPool(m_context->m_device, m_handle, nullptr);
}

VkCommandBuffer vren::command_pool::create_object()
{
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = m_handle;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd_buf;
    vren::vk_utils::check(vkAllocateCommandBuffers(m_context->m_device, &alloc_info, &cmd_buf));
    return cmd_buf;
}

void vren::command_pool::release(VkCommandBuffer const& obj)
{
    vkResetCommandBuffer(obj, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    vren::object_pool<VkCommandBuffer>::release(obj);
}
