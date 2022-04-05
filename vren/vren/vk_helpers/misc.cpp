#include "misc.hpp"

#include "context.hpp"
#include "toolbox.hpp"
#include "pools/command_pool.hpp"
#include "pools/fence_pool.hpp"

void vren::vk_utils::check(VkResult result)
{
	if (result != VK_SUCCESS)
	{
		printf("Vulkan command failed with code: %d\n", result);
		fflush(stdout);

		throw std::runtime_error("Vulkan command failed");
	}
}

vren::vk_semaphore vren::vk_utils::create_semaphore(vren::context const& ctx)
{
	VkSemaphoreCreateInfo sem_info{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL
	};

	VkSemaphore sem;
	vren::vk_utils::check(vkCreateSemaphore(ctx.m_device, &sem_info, nullptr, &sem));
	return vren::vk_semaphore(ctx, sem);
}

vren::vk_fence vren::vk_utils::create_fence(vren::context const& ctx, bool signaled)
{
	VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlags) 0
    };

	VkFence fence;
	vren::vk_utils::check(vkCreateFence(ctx.m_device, &fence_info, nullptr, &fence));
	return vren::vk_fence(ctx, fence);
}

void vren::vk_utils::immediate_submit(
	vren::context const& ctx,
	vren::command_pool& cmd_pool,
	vren::fence_pool& fence_pool,
	VkQueue queue,
	record_commands_func_t const& record_func
)
{
	auto cmd_buf = cmd_pool.acquire();

    vren::resource_container res_container;

    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf.m_handle, &begin_info));

	record_func(cmd_buf.m_handle, res_container);

    vren::vk_utils::check(vkEndCommandBuffer(cmd_buf.m_handle));

	auto fence = fence_pool.acquire();

    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf.m_handle,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };
    vren::vk_utils::check(vkQueueSubmit(queue, 1, &submit_info, fence.m_handle.m_handle));

    vren::vk_utils::check(vkWaitForFences(ctx.m_device, 1, &fence.m_handle.m_handle, VK_TRUE, UINT64_MAX));
}

void vren::vk_utils::immediate_graphics_queue_submit(
	vren::context const& ctx,
	record_commands_func_t const& record_func
)
{
    vren::vk_utils::immediate_submit(
		ctx,
		ctx.m_toolbox->m_graphics_command_pool,
		ctx.m_toolbox->m_fence_pool,
		ctx.m_graphics_queue,
		record_func
	);
}

void vren::vk_utils::immediate_transfer_queue_submit(
	vren::context const& ctx,
	record_commands_func_t const& record_func
)
{
	vren::vk_utils::immediate_submit(
		ctx,
		ctx.m_toolbox->m_transfer_command_pool,
		ctx.m_toolbox->m_fence_pool,
		ctx.m_transfer_queue,
		record_func
	);
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

vren::vk_query_pool vren::vk_utils::create_timestamp_query_pool(
	vren::context const& ctx,
	uint32_t query_count
)
{
	VkQueryPoolCreateInfo pool_info{
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.queryType = VK_QUERY_TYPE_TIMESTAMP,
		.queryCount = query_count,
		.pipelineStatistics = NULL
	};

	VkQueryPool handle;
	vren::vk_utils::check(vkCreateQueryPool(ctx.m_device, &pool_info, nullptr, &handle));
	return vren::vk_query_pool(ctx, handle);
}

std::vector<VkQueueFamilyProperties> vren::vk_utils::get_queue_families_properties(VkPhysicalDevice phy_dev)
{
	uint32_t queue_fam_props_cnt;
	vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &queue_fam_props_cnt, nullptr);

	std::vector<VkQueueFamilyProperties> queue_fam_props(queue_fam_props_cnt);
	vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &queue_fam_props_cnt, queue_fam_props.data());

	return queue_fam_props;
}