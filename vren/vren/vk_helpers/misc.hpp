#pragma once

#include <memory>
#include <functional>

#include "vk_raii.hpp"
#include "base/resource_container.hpp"
#include "pool/command_pool.hpp"

#define VREN_CHECK(...) vren::vk_utils::check(__VA_ARGS__)

namespace vren
{
	// Forward decl
	class context;
}

namespace vren::vk_utils
{
	void check(VkResult result, vren::context const* context);
	void check(VkResult result);

	vren::vk_semaphore create_semaphore(vren::context const& ctx);

	vren::vk_fence create_fence(vren::context const& ctx, bool signaled = false);

	// ------------------------------------------------------------------------------------------------
	// Immediate submission
	// ------------------------------------------------------------------------------------------------

	using record_commands_func_t =
		std::function<void(VkCommandBuffer cmd_buf, vren::resource_container& res_container)>;

	void immediate_submit(
		vren::context const& ctx,
		vren::command_pool& cmd_pool,
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

	// ------------------------------------------------------------------------------------------------

	inline void write_buffer_descriptor(
		vren::context const& context,
		VkDescriptorSet descriptor_set,
		uint32_t binding,
		VkBuffer buffer,
		uint32_t range = VK_WHOLE_SIZE,
		uint32_t offset = 0
	)
	{
		VkDescriptorBufferInfo buffer_info{
			.buffer = buffer, .offset = offset, .range = range,
		};
		VkWriteDescriptorSet descriptor_set_write{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = descriptor_set,
			.dstBinding = binding,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = &buffer_info,
			.pTexelBufferView = nullptr
		};
		vkUpdateDescriptorSets(context.m_device, 1, &descriptor_set_write, 0, nullptr);
	}

}
