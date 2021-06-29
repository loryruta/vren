#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace vren
{
	class renderer;

	//

	struct image
	{
		VkFormat m_format = VK_FORMAT_UNDEFINED;

		VkImage m_handle = VK_NULL_HANDLE;
		VmaAllocation m_allocation = VK_NULL_HANDLE;
	};

	void create_image(vren::renderer& renderer, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, vren::image& result);
	void destroy_image(vren::renderer& renderer, vren::image& result);

	struct image_view
	{
		VkImageView m_handle = VK_NULL_HANDLE;
	};

	void create_image_view(vren::renderer& renderer, vren::image const& image, VkImageAspectFlagBits aspect, vren::image_view& result);
	void destroy_image_view(vren::renderer& renderer, vren::image_view& result);
}
