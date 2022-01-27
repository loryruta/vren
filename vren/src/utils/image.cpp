#include "image.hpp"

#include "buffer.hpp"
#include "utils/misc.hpp"

void vren::vk_utils::begin_single_submit_command_buffer(
	vren::vk_command_buffer const& cmd_buf
)
{
	VkCommandBufferBeginInfo cmd_buf_begin_info{};
	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;
	vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf.m_handle, &cmd_buf_begin_info));
}

void vren::vk_utils::end_single_submit_command_buffer(
	vren::vk_command_buffer const&& cmd_buf,
	VkQueue queue
)
{
	vren::vk_utils::check(vkEndCommandBuffer(cmd_buf.m_handle));

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf.m_handle;
	vren::vk_utils::check(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

	vren::vk_utils::check(vkQueueWaitIdle(queue)); // todo wait just on the current transmission through a fence
}

void vren::vk_utils::immediate_submit(
	vren::context const& ctx,
	std::function<void(vren::vk_command_buffer const&)> submit_func
)
{
	auto cmd_buf = ctx.m_graphics_command_pool->acquire_command_buffer();
	vren::vk_utils::begin_single_submit_command_buffer(cmd_buf);

	submit_func(cmd_buf);

	vren::vk_utils::end_single_submit_command_buffer(std::move(cmd_buf), ctx.m_graphics_queue);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Image
// --------------------------------------------------------------------------------------------------------------------------------

void vren::create_image(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t width,
	uint32_t height,
	void* image_data,
	VkFormat format,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memory_properties,
	vren::image& result
)
{
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
	image_info.usage = usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.requiredFlags = memory_properties;

	VkImage image;
	VmaAllocation allocation;
	vren::vk_utils::check(vmaCreateImage(ctx->m_vma_allocator, &image_info, &alloc_create_info, &image, &allocation, nullptr));

	result.m_image = std::make_shared<vren::vk_image>(ctx, image);
	result.m_allocation = std::make_shared<vren::vma_allocation>(ctx, allocation);

	if (image_data)
	{
		VkMemoryRequirements image_memory_requirements{};
		vkGetImageMemoryRequirements(ctx->m_device, result.m_image->m_handle, &image_memory_requirements);
		VkDeviceSize image_size = image_memory_requirements.size;

		vren::vk_utils::buffer staging_buffer =
			vren::vk_utils::alloc_host_visible_buffer(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image_size, false);

		void* mapped_image_data;
		vren::vk_utils::check(vmaMapMemory(ctx->m_vma_allocator, staging_buffer.m_allocation->m_handle, &mapped_image_data));
			memcpy(mapped_image_data, image_data, static_cast<size_t>(image_size));
		vmaUnmapMemory(ctx->m_vma_allocator, staging_buffer.m_allocation->m_handle);

		auto cmd_buf = ctx->m_graphics_command_pool->acquire_command_buffer();
		vren::vk_utils::begin_single_submit_command_buffer(cmd_buf);

		// transition from undefined to transfer layout
		{
			VkImageMemoryBarrier memory_barrier{};
			memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.srcAccessMask = NULL;
			memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memory_barrier.image = result.m_image->m_handle;
			memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			memory_barrier.subresourceRange.baseMipLevel = 0;
			memory_barrier.subresourceRange.levelCount = 1;
			memory_barrier.subresourceRange.baseArrayLayer = 0;
			memory_barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(cmd_buf.m_handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &memory_barrier);
		}

		// copies staging buffer to image
		{
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

			vkCmdCopyBufferToImage(cmd_buf.m_handle, staging_buffer.m_buffer->m_handle, result.m_image->m_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		// transition from transfer to shader readonly optimal layout
		{
			VkImageMemoryBarrier memory_barrier{};
			memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			memory_barrier.image = result.m_image->m_handle;
			memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			memory_barrier.subresourceRange.baseMipLevel = 0;
			memory_barrier.subresourceRange.levelCount = 1;
			memory_barrier.subresourceRange.baseArrayLayer = 0;
			memory_barrier.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(cmd_buf.m_handle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, NULL, 0, nullptr, 0, nullptr, 1, &memory_barrier);
		}

		vren::vk_utils::end_single_submit_command_buffer(std::move(cmd_buf), ctx->m_transfer_queue);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------
// Image view
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_image_view vren::create_image_view(
	std::shared_ptr<vren::context> const& renderer,
	VkImage image,
	VkFormat format,
	VkImageAspectFlagBits aspect
)
{
	VkImageViewCreateInfo image_view_info{};
	image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	image_view_info.image = image;
	image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	image_view_info.format = format;
	image_view_info.subresourceRange.aspectMask = aspect;
	image_view_info.subresourceRange.baseMipLevel = 0;
	image_view_info.subresourceRange.levelCount = 1;
	image_view_info.subresourceRange.baseArrayLayer = 0;
	image_view_info.subresourceRange.layerCount = 1;

	VkImageView image_view;
	vren::vk_utils::check(vkCreateImageView(renderer->m_device, &image_view_info, nullptr, &image_view));

	return vren::vk_image_view(renderer, image_view);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Sampler
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_sampler vren::create_sampler(
	std::shared_ptr<vren::context> const& ctx,
	VkFilter mag_filter,
	VkFilter min_filter,
	VkSamplerMipmapMode mipmap_mode,
	VkSamplerAddressMode address_mode_u,
	VkSamplerAddressMode address_mode_v,
	VkSamplerAddressMode address_mode_w
)
{
	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.pNext = nullptr;
	sampler_info.flags = NULL;
	sampler_info.minFilter = min_filter;
	sampler_info.magFilter = mag_filter;
	sampler_info.mipmapMode = mipmap_mode;
	sampler_info.addressModeU = address_mode_u;
	sampler_info.addressModeV = address_mode_v;
	sampler_info.addressModeW = address_mode_w;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.anisotropyEnable = 0.0f;
	sampler_info.maxAnisotropy = 1.0f;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;

	VkSampler sampler;
	vren::vk_utils::check(vkCreateSampler(ctx->m_device, &sampler_info, nullptr, &sampler));

	return vren::vk_sampler(ctx, sampler);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Texture
// --------------------------------------------------------------------------------------------------------------------------------

void vren::create_texture(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t width,
	uint32_t height,
	void* image_data,
	VkFormat format,
	VkFilter mag_filter,
	VkFilter min_filter,
	VkSamplerMipmapMode mipmap_mode,
	VkSamplerAddressMode address_mode_u,
	VkSamplerAddressMode address_mode_v,
	VkSamplerAddressMode address_mode_w,
	vren::texture& result
)
{
	// Image
	vren::image image;
	vren::create_image(
		ctx,
		width,
		height,
		image_data,
		format,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		image
	);
	result.m_image = image.m_image;
	result.m_image_allocation = image.m_allocation;

	// Image view
	result.m_image_view = std::make_shared<vren::vk_image_view>(
		vren::create_image_view(
			ctx,
			result.m_image->m_handle,
			format,
			VK_IMAGE_ASPECT_COLOR_BIT
		)
	);

	// Sampler
	result.m_sampler = std::make_shared<vren::vk_sampler>(
		vren::create_sampler(
			ctx,
			mag_filter,
			min_filter,
			mipmap_mode,
			address_mode_u,
			address_mode_v,
			address_mode_w
		)
	);
}

void vren::create_color_texture(
	std::shared_ptr<vren::context> const& ctx,
	uint8_t r,
	uint8_t g,
	uint8_t b,
	uint8_t a,
	vren::texture& result
)
{
	std::vector<uint8_t> img_data = {r, g, b, a};

	vren::create_texture(
		ctx,
		1,
		1,
		img_data.data(),
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FILTER_NEAREST, // mag filter
		VK_FILTER_NEAREST, // min filter
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		result
	);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Framebuffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_framebuffer vren::create_framebuffer(
	std::shared_ptr<vren::context> const& ctx,
	VkRenderPass render_pass,
	std::vector<VkImageView> const& attachments,
	VkExtent2D size
)
{
	VkFramebufferCreateInfo fb_info{};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.renderPass = render_pass;
	fb_info.attachmentCount = (uint32_t) attachments.size();
	fb_info.pAttachments = attachments.data();
	fb_info.width = size.width;
	fb_info.height = size.height;
	fb_info.layers = 1;

	VkFramebuffer fb;
	vren::vk_utils::check(vkCreateFramebuffer(ctx->m_device, &fb_info, nullptr, &fb));
	return vren::vk_framebuffer(ctx, fb);
}
