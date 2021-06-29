#include "vk_utils.hpp"

#include "renderer.hpp"

void vren::create_image(vren::renderer& renderer, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, vren::image& result)
{
	result.m_format = format;

	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(renderer.m_device, &image_info, nullptr, &result.m_handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image.");
	}

	VkMemoryRequirements mem_req{};
	vkGetImageMemoryRequirements(renderer.m_device, result.m_handle, &mem_req);

	VmaAllocationCreateInfo alloc_info{};
	alloc_info.requiredFlags = memory_properties;
	alloc_info.memoryTypeBits = 0; // TODO findMemoryType(mem_req.memoryTypeBits, mem_req)

	if (vmaAllocateMemory(renderer.m_allocator, &mem_req, &alloc_info, &result.m_allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image allocation.");
	}

	vmaBindImageMemory(renderer.m_allocator, result.m_allocation, result.m_handle);
}

void vren::destroy_image(vren::renderer& renderer, vren::image& result)
{
	if (result.m_handle != VK_NULL_HANDLE)
	{
		vmaDestroyImage(renderer.m_allocator, result.m_handle, nullptr);
		result.m_handle = VK_NULL_HANDLE;
	}

	if (result.m_allocation != VK_NULL_HANDLE)
	{
		vmaFreeMemory(renderer.m_allocator, result.m_allocation);
		result.m_allocation = VK_NULL_HANDLE;
	}
}

void vren::create_image_view(vren::renderer& renderer, vren::image const& image, VkImageAspectFlagBits aspect, vren::image_view& result)
{
	VkImageViewCreateInfo image_view_info{};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.image = image.m_handle;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = image.m_format;
	image_view_info.subresourceRange.aspectMask = aspect;
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(renderer.m_device, &image_view_info, nullptr, &result.m_handle) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the image view.");
	}
}

void vren::destroy_image_view(vren::renderer& renderer, vren::image_view& result)
{
	if (result.m_handle != VK_NULL_HANDLE)
	{
		vkDestroyImageView(renderer.m_device, result.m_handle, nullptr);
		result.m_handle = VK_NULL_HANDLE;
	}
}


