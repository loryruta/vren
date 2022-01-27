#include "misc.hpp"

void vren::vk_utils::check(VkResult result)
{
	if (result != VK_SUCCESS)
	{
		printf("Vulkan command failed with code: %d\n", result);
		fflush(stdout);

		throw std::runtime_error("Vulkan command failed");
	}
}

vren::vk_semaphore vren::vk_utils::create_semaphore(
	std::shared_ptr<vren::context> const& ctx
)
{
	VkSemaphoreCreateInfo sem_info{};
	sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore sem;
	vren::vk_utils::check(vkCreateSemaphore(ctx->m_device, &sem_info, nullptr, &sem));
	return vren::vk_semaphore(ctx, sem);
}

vren::vk_fence vren::vk_utils::create_fence(
	std::shared_ptr<vren::context> const& ctx
)
{
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;
	vren::vk_utils::check(vkCreateFence(ctx->m_device, &fence_info, nullptr, &fence));
	return vren::vk_fence(ctx, fence);
}

void vren::vk_utils::submit_command_buffer(
	VkQueue queue,
	std::vector<VkSemaphore> const& wait_semaphores,
	std::vector<VkPipelineStageFlags> const& wait_stages,
	vren::vk_command_buffer const& cmd_buf,
	std::vector<VkSemaphore> const& signal_semaphores,
	VkFence signal_fence
)
{
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = wait_semaphores.size();
	submit_info.pWaitSemaphores = wait_semaphores.data();
	submit_info.pWaitDstStageMask = wait_stages.data();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf.m_handle;
	submit_info.signalSemaphoreCount = signal_semaphores.size();
	submit_info.pSignalSemaphores = signal_semaphores.data();

	vren::vk_utils::check(vkQueueSubmit(queue, 1, &submit_info, signal_fence));
}
