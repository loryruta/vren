#pragma once

#include <memory>
#include <functional>

#include "context.hpp"
#include "vk_raii.hpp"
#include "resource_container.hpp"

namespace vren::vk_utils
{
	void check(VkResult result);

	vren::vk_semaphore create_semaphore(
		std::shared_ptr<vren::context> const& ctx
	);

	vren::vk_fence create_fence(std::shared_ptr<vren::context> const& ctx, bool signaled = false);


    using record_commands_func_t =
        std::function<void(VkCommandBuffer cmd_buf, vren::resource_container& res_container)>;

	void record_one_time_submit_commands(VkCommandBuffer cmd_buf, std::function<void(VkCommandBuffer cmd_buf)> const& record_func); // TODO use it below

	void immediate_submit(vren::context const& ctx, vren::command_pool& cmd_pool, VkQueue queue, record_commands_func_t const& record_func);

    void immediate_graphics_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);
    //void immediate_compute_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);
    void immediate_transfer_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);

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

	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_query_pool create_timestamp_query_pool(std::shared_ptr<vren::context> const& ctx, uint32_t query_count);

	std::vector<VkQueueFamilyProperties> get_queue_families_properties(VkPhysicalDevice phy_dev);
}
