#include "fence_pool.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

vren::fence_pool::fence_pool(std::shared_ptr<vren::context> ctx) :
	m_context(std::move(ctx))
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
		vren::vk_fence(m_context, fence)
	);
}

std::shared_ptr<vren::fence_pool> vren::fence_pool::create(std::shared_ptr<vren::context> const& ctx)
{
	return std::shared_ptr<vren::fence_pool>(new vren::fence_pool(ctx));
}

