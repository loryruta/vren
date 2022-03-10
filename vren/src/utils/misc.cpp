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

void vren::vk_utils::begin_single_submit_command_buffer(
	vren::vk_command_buffer const& cmd_buf
)
{
	VkCommandBufferBeginInfo cmd_buf_begin_info{};
	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;

	vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf.m_handle, &cmd_buf_begin_info));
}

void vren::vk_utils::end_single_submit_command_buffer(
	vren::vk_command_buffer const&& cmd_buf,
	VkQueue queue
)
{
	vren::vk_utils::check(vkEndCommandBuffer(cmd_buf.m_handle));

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf.m_handle;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	vren::vk_utils::check(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	vren::vk_utils::check(vkQueueWaitIdle(queue)); // todo wait just on the current transmission through a fence
}

void vren::vk_utils::immediate_submit(
	vren::context const& ctx,
	std::function<void(vren::vk_command_buffer const&)> submit_func
)
{
	auto cmd_buf = ctx.m_graphics_command_pool->acquire_command_buffer();
	vren::vk_utils::begin_single_submit_command_buffer(cmd_buf);

	submit_func(cmd_buf);

	vren::vk_utils::end_single_submit_command_buffer(std::move(cmd_buf), ctx.m_graphics_queue);
}

vren::vk_utils::surface_details vren::vk_utils::get_surface_details(
	VkPhysicalDevice physical_device,
	VkSurfaceKHR surface
)
{
	vren::vk_utils::surface_details surf_det{};

	// Surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surf_det.m_capabilities);

	// Surface formats
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

	if (format_count != 0)
	{
		surf_det.m_surface_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, surf_det.m_surface_formats.data());
	}

	// Present modes
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0)
	{
		surf_det.m_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, surf_det.m_present_modes.data());
	}

	return surf_det;
}
