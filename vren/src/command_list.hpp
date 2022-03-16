#pragma once

#include "context.hpp"
#include "pooling/command_pool.hpp"
#include "pooling/semaphore_pool.hpp"
#include "resource_container.hpp"

namespace vren
{
    class command_list : public std::enable_shared_from_this<command_list>
    {
    public:
        class node
        {
            friend command_list;

        private:
            std::shared_ptr<vren::command_list> m_command_list;

            vren::pooled_vk_command_buffer m_command_buffer;

            std::vector<VkSemaphore> m_wait_semaphores;
            std::vector<VkSemaphore> m_signal_semaphores;

        public:
            explicit node(
                std::shared_ptr<vren::command_list> cmd_list,
                vren::pooled_vk_command_buffer&& cmd_buf
            );

            void wait_on(node& node);
        };

    private:
        std::shared_ptr<vren::command_pool> m_command_pool;
        std::shared_ptr<vren::semaphore_pool> m_semaphore_pool;

        std::vector<vren::pooled_vk_semaphore> m_semaphores;

        std::vector<node> m_nodes;

    public:
        command_list(
            std::shared_ptr<vren::command_pool> cmd_pool,
            std::shared_ptr<vren::semaphore_pool> sem_pool
        );

        node& create_node();

        void submit(VkQueue queue, uint32_t semaphores_count = 0, VkSemaphore* semaphores = nullptr, VkFence fence = VK_NULL_HANDLE);
    };
}

