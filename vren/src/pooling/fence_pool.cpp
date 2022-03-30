#include "fence_pool.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

vren::fence_pool::fence_pool(std::shared_ptr<vren::context> ctx) :
	m_context(std::move(ctx))
{}

vren::vk_fence vren::fence_pool::create_object()
{
	VkFenceCreateInfo fence_info{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
	};
	VkFence fence;
	vren::vk_utils::check(vkCreateFence(m_context->m_device, &fence_info, nullptr, &fence));
	return vren::vk_fence(m_context, fence);
}

vren::pooled_vk_fence vren::fence_pool::acquire()
{
	auto fence = vren::object_pool<vren::vk_fence>::acquire();
	vren::vk_utils::check(vkResetFences(m_context->m_device, 1, &fence.m_handle.m_handle));
	return fence;
}

std::shared_ptr<vren::fence_pool> vren::fence_pool::create(std::shared_ptr<vren::context> const& ctx)
{
	return std::shared_ptr<vren::fence_pool>(new vren::fence_pool(ctx));
}

