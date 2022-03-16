#pragma once

#include <vulkan/vulkan.h>

#include "context.hpp"
#include "object_pool.hpp"
#include "utils/misc.hpp"

namespace vren
{
    using pooled_vk_fence = vren::pooled_object<VkFence>;

    class fence_pool : public vren::object_pool<VkFence>
    {
    private:
        std::shared_ptr<vren::context> m_context;

    protected:
        VkFence create_object() override
        {
            VkFenceCreateInfo fence_info{};
            fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fence_info.pNext = nullptr;
            fence_info.flags = NULL;

            VkFence fence;
            vren::vk_utils::check(vkCreateFence(m_context->m_device, &fence_info, nullptr, &fence));
            return fence;
        }

    public:
        explicit fence_pool(std::shared_ptr<vren::context> ctx) :
            m_context(std::move(ctx))
        {}

        vren::pooled_vk_fence acquire() override
        {
            auto fence = vren::object_pool<VkFence>::acquire();
            vren::vk_utils::check(vkResetFences(m_context->m_device, 1, &fence.m_handle));

            return fence;
        }
    };
}
