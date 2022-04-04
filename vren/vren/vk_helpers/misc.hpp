#pragma once

#include <memory>
#include <functional>

#include "vk_raii.hpp"
#include "base/resource_container.hpp"
#include "pools/command_pool.hpp"
#include "pools/fence_pool.hpp"

namespace vren
{
	// Forward decl
	class context;
}

namespace vren::vk_utils
{
	void check(VkResult result);

	vren::vk_semaphore create_semaphore(vren::context const& ctx);

	vren::vk_fence create_fence(vren::context const& ctx,bool signaled = false);

	// ------------------------------------------------------------------------------------------------
	// Immediate submission
	// ------------------------------------------------------------------------------------------------

	using record_commands_func_t =
		std::function<void(VkCommandBuffer cmd_buf, vren::resource_container& res_container)>;

	void immediate_submit(
		vren::context const& ctx,
		vren::command_pool& cmd_pool,
		vren::fence_pool& fence_pool,
		VkQueue queue,
		record_commands_func_t const& record_func
	);

    void immediate_graphics_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);
	void immediate_transfer_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);

	// ------------------------------------------------------------------------------------------------
	// Surface details
	// ------------------------------------------------------------------------------------------------

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

	vren::vk_query_pool create_timestamp_query_pool(
		vren::context const& ctx,
		uint32_t query_count
	);

	std::vector<VkQueueFamilyProperties> get_queue_families_properties(VkPhysicalDevice phy_dev);
}
