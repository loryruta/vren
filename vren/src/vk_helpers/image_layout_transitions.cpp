#include "image_layout_transitions.hpp"

void vren::vk_utils::transition_image_layout_undefined_to_transfer_dst(VkCommandBuffer cmd_buf, VkImage img)
{
    VkImageMemoryBarrier mem_barrier{};
    mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mem_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mem_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    mem_barrier.srcAccessMask = NULL;
    mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.image = img;
    mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mem_barrier.subresourceRange.baseMipLevel = 0;
    mem_barrier.subresourceRange.levelCount = 1;
    mem_barrier.subresourceRange.baseArrayLayer = 0;
    mem_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &mem_barrier);
}

void vren::vk_utils::transition_image_layout_transfer_dst_to_shader_readonly(VkCommandBuffer cmd_buf, VkImage img)
{
    VkImageMemoryBarrier mem_barrier{};
    mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mem_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    mem_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mem_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.image = img;
    mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mem_barrier.subresourceRange.baseMipLevel = 0;
    mem_barrier.subresourceRange.levelCount = 1;
    mem_barrier.subresourceRange.baseArrayLayer = 0;
    mem_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &mem_barrier);
}

void vren::vk_utils::transition_image_layout_undefined_to_color_attachment(VkCommandBuffer cmd_buf, VkImage img)
{
    VkImageMemoryBarrier mem_barrier{};
    mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mem_barrier.pNext = nullptr;
    mem_barrier.srcAccessMask = NULL;
    mem_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    mem_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mem_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.image = img;
    mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mem_barrier.subresourceRange.baseMipLevel = 0;
    mem_barrier.subresourceRange.levelCount = 1;
    mem_barrier.subresourceRange.baseArrayLayer = 0;
    mem_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, NULL, 0, nullptr, 0, nullptr, 1, &mem_barrier);
}

void vren::vk_utils::transition_image_layout_undefined_to_depth_stencil_attachment(VkCommandBuffer cmd_buf, VkImage img)
{
    VkImageMemoryBarrier mem_barrier{};
    mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mem_barrier.pNext = nullptr;
    mem_barrier.srcAccessMask = NULL;
    mem_barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    mem_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mem_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.image = img;
    mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    mem_barrier.subresourceRange.baseMipLevel = 0;
    mem_barrier.subresourceRange.levelCount = 1;
    mem_barrier.subresourceRange.baseArrayLayer = 0;
    mem_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, NULL, 0, nullptr, 0, nullptr, 1, &mem_barrier);
}

void vren::vk_utils::transition_color_attachment_to_shader_readonly(VkCommandBuffer cmd_buf, VkImage image)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, NULL, 0, nullptr, 0, nullptr, 1, &barrier);
}

void vren::vk_utils::transition_image_layout_color_attachment_to_present(VkCommandBuffer cmd_buf, VkImage img)
{
    VkImageMemoryBarrier mem_barrier{};
    mem_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mem_barrier.pNext = nullptr;
    mem_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    mem_barrier.dstAccessMask = NULL;
    mem_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    mem_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.image = img;
    mem_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mem_barrier.subresourceRange.baseMipLevel = 0;
    mem_barrier.subresourceRange.levelCount = 1;
    mem_barrier.subresourceRange.baseArrayLayer = 0;
    mem_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, NULL, 0, nullptr, 0, nullptr, 1, &mem_barrier);
}

