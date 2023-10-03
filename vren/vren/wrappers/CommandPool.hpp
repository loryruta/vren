#pragma once

#include <vector>

#include <volk.h>

namespace vren
{
    // Forward decl
    class CommandPool;
    class PooledCommandBuffer;

    // ------------------------------------------------------------------------------------------------ PooledCommandBuffer

    class PooledCommandBuffer
    {
        friend class CommandPool;

    private:
        CommandPool& m_command_pool;
        VkCommandBuffer m_handle;

        explicit PooledCommandBuffer(CommandPool& command_pool, VkCommandBuffer handle);

    public:
        PooledCommandBuffer(PooledCommandBuffer const& other) = delete;
        PooledCommandBuffer(PooledCommandBuffer&& other) noexcept;
        ~PooledCommandBuffer();
    };

    // ------------------------------------------------------------------------------------------------ CommandPool

    class CommandPool
    {
        friend class PooledCommandBuffer;

    private:
        HandleDeleter<VkCommandPool> m_handle;
        std::vector<VkCommandBuffer> m_unused_command_buffers;

        /// \param handle A VkCommandPool that must be created with VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT set.
        explicit CommandPool(VkCommandPool handle);

    public:
        PooledCommandBuffer acquire();

        static CommandPool create();
    };
} // namespace vren
