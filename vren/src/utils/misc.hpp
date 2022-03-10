#pragma once

#include <memory>
#include <functional>

#include "context.hpp"
#include "vk_raii.hpp"
#include "command_pool.hpp"

namespace vren::vk_utils
{
	void check(VkResult result);

	vren::vk_semaphore create_semaphore(
		std::shared_ptr<vren::context> const& ctx
	);

	vren::vk_fence create_fence(
		std::shared_ptr<vren::context> const& ctx
	);

	void submit_command_buffer(
		VkQueue queue,
		std::vector<VkSemaphore> const& wait_semaphores,
		std::vector<VkPipelineStageFlags> const& wait_stages,
		vren::vk_command_buffer const& cmd_buf,
		std::vector<VkSemaphore> const& signal_semaphores,
		VkFence signal_fence
	);

	void begin_single_submit_command_buffer(
		vren::vk_command_buffer const& cmd_buf
	);

	void end_single_submit_command_buffer(
		vren::vk_command_buffer const&& cmd_buf,
		VkQueue queue
	);

	/** Immediately submits a command buffer on the graphics queue. */
	void immediate_submit(
		vren::context const& ctx,
		std::function<void(vren::vk_command_buffer const&)> submit_func
	);

	// --------------------------------------------------------------------------------------------------------------------------------
	// Surface details
	// --------------------------------------------------------------------------------------------------------------------------------

	struct surface_details
	{
		VkSurfaceCapabilitiesKHR m_capabilities;
		std::vector<VkPresentModeKHR> m_present_modes;
		std::vector<VkSurfaceFormatKHR> m_surface_formats;
	};

	vren::vk_utils::surface_details get_surface_details(
		VkPhysicalDevice physical_device,
		VkSurfaceKHR surface
	);
}
