#pragma once

#include <vulkan/vulkan.h>

namespace vren::vk_utils
{
	void buffer_barrier(
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

}
