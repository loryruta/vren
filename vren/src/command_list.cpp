#include "command_list.hpp"

#include <utility>

vren::command_list::node::node(
    std::shared_ptr<vren::command_list> cmd_list,
    vren::pooled_vk_command_buffer&& cmd_buf
) :
    m_command_list(std::move(cmd_list)),
    m_command_buffer(std::move(cmd_buf))
{
}

void vren::command_list::node::wait_on(node& node)
{
    auto sem = m_command_list->m_semaphore_pool->acquire();

    node.m_signal_semaphores.push_back(sem.m_handle);
    m_wait_semaphores.push_back(sem.m_handle);

    m_command_list->m_semaphores.push_back(std::move(sem));
}

vren::command_list::command_list(
    std::shared_ptr<vren::command_pool> cmd_pool,
    std::shared_ptr<vren::semaphore_pool> sem_pool
) :
    m_command_pool(std::move(cmd_pool)),
    m_semaphore_pool(std::move(sem_pool))
{
}

vren::command_list::node& vren::command_list::create_node()
{
    return m_nodes.emplace_back(
        shared_from_this(),
        m_command_pool->acquire()
    );
}

void vren::command_list::submit(VkQueue queue, uint32_t semaphores_count, VkSemaphore *semaphores, VkFence fence)
{
    std::vector<VkSubmitInfo> submits{};
    std::vector<VkSemaphore> end_semaphores{};

    for (auto& node : m_nodes)
    {
        std::vector<VkSemaphore> signal_semaphores = node.m_signal_semaphores;

        if (node.m_signal_semaphores.empty())
        {
            auto sem = m_semaphore_pool->acquire();
            signal_semaphores.push_back(sem.m_handle);
            end_semaphores.push_back(sem.m_handle);
        }

        submits.push_back({
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = (uint32_t) node.m_wait_semaphores.size(),
            .pWaitSemaphores = node.m_wait_semaphores.data(),
            .commandBufferCount = 1,
            .pCommandBuffers = &node.m_command_buffer.m_handle,
            .signalSemaphoreCount = (uint32_t) signal_semaphores.size(),
            .pSignalSemaphores = signal_semaphores.data(),
        });
    }

    submits.push_back({
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t) end_semaphores.size(),
        .pWaitSemaphores = end_semaphores.data(),
        .commandBufferCount = 0,
        .pCommandBuffers = nullptr,
        .signalSemaphoreCount = semaphores_count,
        .pSignalSemaphores = semaphores,
    });

    vren::vk_utils::check(vkQueueSubmit(queue, submits.size(), submits.data(), fence));
}


