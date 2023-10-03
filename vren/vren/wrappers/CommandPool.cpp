#include "CommandPool.hpp"

#include "Context.hpp"
#include "vk_helpers/misc.hpp"

using namespace vren;

// ------------------------------------------------------------------------------------------------ PooledCommandBuffer

PooledCommandBuffer::PooledCommandBuffer(CommandPool& command_pool, VkCommandBuffer handle) :
    m_command_pool(command_pool),
    m_handle(handle)
{
}

PooledCommandBuffer::PooledCommandBuffer(PooledCommandBuffer&& other) noexcept :
    m_command_pool(other.m_command_pool),
    m_handle(other.m_handle)
{
    other.m_handle = VK_NULL_HANDLE;
}

PooledCommandBuffer::~PooledCommandBuffer()
{
    if (m_handle != VK_NULL_HANDLE)
        m_command_pool.m_unused_command_buffers.push_back(m_handle);
}

// ------------------------------------------------------------------------------------------------ CommandPool

CommandPool::CommandPool(VkCommandPool handle) :
    m_handle(handle)
{
}

PooledCommandBuffer CommandPool::acquire()
{
    if (m_unused_command_buffers.size() > 0)
    {
        VkCommandBuffer unused_command_buffer = m_unused_command_buffers.back();
        m_unused_command_buffers.pop_back();

        vkResetCommandBuffer(unused_command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        return PooledCommandBuffer(*this, unused_command_buffer);
    }

    VkCommandBufferAllocateInfo command_buffer_alloc_info{};
    command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.commandPool = m_handle;
    command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer_handle;
    VREN_CHECK(vkAllocateCommandBuffers(Context::get().device().handle(), &command_buffer_alloc_info, &command_buffer_handle));
    return PooledCommandBuffer(*this, command_buffer_handle);
}

CommandPool CommandPool::create()
{
    Context& context = Context::get();

    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = context.m_queue_family_idx;

    VkCommandPool handle;
    VREN_CHECK(vkCreateCommandPool(context.device().handle(), &command_pool_info, nullptr, &handle));
    return CommandPool(handle);
}
