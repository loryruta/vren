#pragma once

#include <volk.h>

#define VREN_DECLARE_IMAGE_BARRIER(_function_name) \
\
void _function_name(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageSubresourceRange subresource_range); \
\
inline void  _function_name(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags image_aspect, uint32_t mipmap_level, uint32_t layer) \
{ \
	_function_name(command_buffer, image, old_layout, new_layout, { .aspectMask = image_aspect, .baseMipLevel = mipmap_level, .levelCount = 1, .baseArrayLayer = layer, .layerCount = 1 });\
}; \
\
inline void _function_name(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags image_aspect, uint32_t mipmap_level) \
{ \
	_function_name(command_buffer, image, old_layout, new_layout, image_aspect, mipmap_level, 0); \
} \
\
inline void _function_name(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags image_aspect) \
{ \
	_function_name(command_buffer, image, old_layout, new_layout, image_aspect, 0); \
}

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

	void image_barrier();


	VREN_DECLARE_IMAGE_BARRIER(image_barrier_before_transfer_stage);
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_after_transfer_stage);
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_after_compute_stage);
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_before_color_attachment_output);
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_after_color_attachment_output_stage_before_transfer_stage)
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_after_transfer_stage_before_color_attachment_output_stage);
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_after_late_fragment_tests_stage_before_compute_stage);
	VREN_DECLARE_IMAGE_BARRIER(image_barrier_after_late_fragment_tests_stage);

	VREN_DECLARE_IMAGE_BARRIER(ensure_depth_buffer_visibility_for_compute_stage);
}
