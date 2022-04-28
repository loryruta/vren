#include "barrier.hpp"

void vren::vk_utils::image_barrier_before_transfer_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}


void vren::vk_utils::image_barrier_after_transfer_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

void vren::vk_utils::image_barrier_after_compute_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
		.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

void vren::vk_utils::image_barrier_after_color_attachment_output_stage_before_transfer_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

void vren::vk_utils::image_barrier_after_transfer_stage_before_color_attachment_output_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{

}

void vren::vk_utils::image_barrier_after_late_fragment_tests_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

void vren::vk_utils::ensure_depth_buffer_visibility_for_transfer_stage(
	VkCommandBuffer command_buffer,
	VkImage depth_buffer,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = depth_buffer,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}

void vren::vk_utils::ensure_depth_buffer_visibility_for_compute_stage(
	VkCommandBuffer command_buffer,
	VkImage image,
	VkImageLayout old_layout, VkImageLayout new_layout,
	VkImageSubresourceRange subresource_range
)
{
	VkImageMemoryBarrier image_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = subresource_range
	};
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &image_barrier);
}
