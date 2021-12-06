#include "vk_utils.hpp"

#include "renderer.hpp"

void vren::create_image(
	vren::renderer& renderer,
	uint32_t width,
	uint32_t height,
	void* image_data,
	VkFormat format,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_properties,
	vren::image& result
)
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

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.requiredFlags = memory_properties;

	if (vmaCreateImage(renderer.m_gpu_allocator.m_allocator, &image_info, &alloc_create_info, &result.m_handle, &result.m_allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image");
	}

	if (image_data)
	{
		auto gpu_allocator = renderer.m_gpu_allocator;

		VkMemoryRequirements image_memory_requirements{};
		vkGetImageMemoryRequirements(renderer.m_device, result.m_handle, &image_memory_requirements);
		VkDeviceSize image_size = image_memory_requirements.size;

		vren::gpu_buffer staging_buffer;
		gpu_allocator.alloc_host_visible_buffer(staging_buffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image_size, false);

		void* mapped_image_data;
		vmaMapMemory(gpu_allocator.m_allocator, staging_buffer.m_allocation, &mapped_image_data);
			memcpy(mapped_image_data, image_data, static_cast<size_t>(image_size));
		vmaUnmapMemory(gpu_allocator.m_allocator, staging_buffer.m_allocation);

		VkCommandBufferAllocateInfo cmd_buf_info{};
		cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buf_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buf_info.commandPool = renderer.m_transfer_command_pool;
		cmd_buf_info.commandBufferCount = 1;

		VkCommandBuffer cmd_buf{};
		vkAllocateCommandBuffers(renderer.m_device, &cmd_buf_info, &cmd_buf);

		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd_buf, &begin_info);

		VkImageMemoryBarrier memory_barrier{};

		memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.srcAccessMask = NULL;
		memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memory_barrier.image = result.m_handle;
		memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memory_barrier.subresourceRange.baseMipLevel = 0;
		memory_barrier.subresourceRange.levelCount = 1;
		memory_barrier.subresourceRange.baseArrayLayer = 0;
		memory_barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &memory_barrier);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(cmd_buf, staging_buffer.m_buffer, result.m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		memory_barrier.image = result.m_handle;
		memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		memory_barrier.subresourceRange.baseMipLevel = 0;
		memory_barrier.subresourceRange.levelCount = 1;
		memory_barrier.subresourceRange.baseArrayLayer = 0;
		memory_barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &memory_barrier);

		vkEndCommandBuffer(cmd_buf);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &cmd_buf;

		VkQueue transfer_queue = renderer.m_transfer_queue;
		vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(transfer_queue);

		//vkFreeCommandBuffers(renderer.m_device, renderer.m_transfer_command_pool, 1, &cmd_buf);
		vkResetCommandPool(renderer.m_device, renderer.m_transfer_command_pool, NULL);
	}
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

void vren::create_texture(
	vren::renderer& renderer,
	uint32_t width,
	uint32_t height,
	void* image_data,
	VkFormat format,
	vren::texture& result
)
{
	vren::create_image(
		renderer,
		width,
		height,
		image_data,
		format,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		result.m_image
	);

	vren::create_image_view(
		renderer,
		result.m_image,
		VK_IMAGE_ASPECT_COLOR_BIT,
		result.m_image_view
	);

	VkSamplerCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = NULL;
	create_info.minFilter = VK_FILTER_LINEAR;
	create_info.magFilter = VK_FILTER_LINEAR;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	create_info.mipLodBias = 0.0f;
	create_info.anisotropyEnable = 0.0f;
	create_info.maxAnisotropy = 1.0f;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.minLod = 0.0f;
	create_info.maxLod = 0.0f;
	create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	create_info.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(renderer.m_device, &create_info, nullptr, &result.m_sampler_handle)) {
		throw std::runtime_error("Failed to create texture sampler");
	}
}




