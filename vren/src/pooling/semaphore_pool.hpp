#pragma once

#include <vulkan/vulkan.h>

#include "context.hpp"
#include "object_pool.hpp"
#include "utils/misc.hpp"

namespace vren
{
    using pooled_vk_semaphore = vren::pooled_object<VkSemaphore>;

    class semaphore_pool : public vren::object_pool<VkSemaphore>
    {
    private:
        std::shared_ptr<vren::context> m_context;

    protected:
        VkSemaphore create_object() override
        {
            VkSemaphoreCreateInfo sem_info{};
            sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            sem_info.pNext = nullptr;
            sem_info.flags = NULL;

            VkSemaphore sem;
            vren::vk_utils::check(vkCreateSemaphore(m_context->m_device, &sem_info, nullptr, &sem));
            return sem;
        }

    public:
        explicit semaphore_pool(std::shared_ptr<vren::context> ctx) :
            m_context(std::move(ctx))
        {}
    };
}

