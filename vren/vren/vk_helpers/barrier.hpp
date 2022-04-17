#pragma once

#include <volk.h>

namespace vren::vk_utils
{
	inline void buffer_barrier(
		VkCommandBuffer cmd_buf,
		VkPipelineStageFlags src_stage_flags,
		VkPipelineStageFlags dst_stage_flags,
		VkAccessFlags src_access_mask,
		VkAccessFlags dst_access_mask,
		VkBuffer buf,
		VkDeviceSize size = VK_WHOLE_SIZE,
		VkDeviceSize off = 0
	)
	{
		VkBufferMemoryBarrier mem_bar{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = src_access_mask,
			.dstAccessMask = dst_access_mask,
			.buffer = buf,
			.offset = off,
			.size = size
		};
		vkCmdPipelineBarrier(
			cmd_buf,
			src_stage_flags,
			dst_stage_flags,
			NULL,
			0, nullptr,
			1, &mem_bar,
			0, nullptr
		);
	}

	inline void image_barrier(
		VkCommandBuffer cmd_buf,
		VkPipelineStageFlags src_stage_flags,
		VkPipelineStageFlags dst_stage_flags,
		VkAccessFlags src_access_mask,
		VkAccessFlags dst_access_mask,
		VkImageLayout old_layout,
		VkImageLayout new_layout,
		VkImage img,
		VkImageAspectFlags aspect_flags
	)
	{
		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = src_access_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = img,
			.subresourceRange = {
				.aspectMask = aspect_flags,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};
		vkCmdPipelineBarrier(
			cmd_buf,
			src_stage_flags,
			dst_stage_flags,
			NULL,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

}
