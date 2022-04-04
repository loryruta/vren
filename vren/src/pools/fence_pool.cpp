#include "fence_pool.hpp"

#include "context.hpp"
#include "vk_helpers/misc.hpp"

vren::fence_pool::fence_pool(vren::context const& ctx) :
	m_context(&ctx)
{}

vren::pooled_vk_fence vren::fence_pool::acquire()
{
	auto pooled = try_acquire();
	if (pooled.has_value())
	{
		vren::vk_utils::check(vkResetFences(m_context->m_device, 1, &pooled.value().m_handle.m_handle));
		return std::move(pooled.value());
	}

	VkFenceCreateInfo fence_info{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
	};
	VkFence fence;
	vren::vk_utils::check(vkCreateFence(m_context->m_device, &fence_info, nullptr, &fence));
	return create_managed_object(
		vren::vk_fence(*m_context, fence)
	);
}
