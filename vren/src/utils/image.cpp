#include "image.hpp"

#include "buffer.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Image
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::image
vren::vk_utils::create_image(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkMemoryPropertyFlags memory_properties,
	VkImageUsageFlags usage,
	VkImageLayout image_layout
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
	image_info.initialLayout = image_layout;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.requiredFlags = memory_properties;

	VkImage image;
	VmaAllocation alloc;
	vren::vk_utils::check(vmaCreateImage(ctx->m_vma_allocator, &image_info, &alloc_create_info, &image, &alloc, nullptr));

	return {
		.m_image = vren::vk_image(ctx, image),
		.m_allocation = vren::vma_allocation(ctx, alloc)
	};
}

void vren::vk_utils::upload_image_data(
	std::shared_ptr<vren::context> const& ctx,
	VkImage img,
	uint32_t img_width,
	uint32_t img_height,
	void* img_data
)
{
	VkMemoryRequirements img_mem_req{};
	vkGetImageMemoryRequirements(ctx->m_device, img, &img_mem_req);
	VkDeviceSize image_size = img_mem_req.size;

	vren::vk_utils::buffer staging_buffer =
		vren::vk_utils::alloc_host_visible_buffer(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, image_size, false);

	void* mapped_img_data;
	vren::vk_utils::check(vmaMapMemory(ctx->m_vma_allocator, staging_buffer.m_allocation.m_handle, &mapped_img_data));
		memcpy(mapped_img_data, img_data, static_cast<size_t>(image_size));
	vmaUnmapMemory(ctx->m_vma_allocator, staging_buffer.m_allocation.m_handle);

	auto cmd_buf = ctx->m_graphics_command_pool->acquire_command_buffer();
	vren::vk_utils::begin_single_submit_command_buffer(cmd_buf);

	transition_image_layout_undefined_to_transfer_dst(ctx, cmd_buf.m_handle, img);

	VkBufferImageCopy img_copy_region{};
	img_copy_region.bufferOffset = 0;
	img_copy_region.bufferRowLength = 0;
	img_copy_region.bufferImageHeight = 0;
	img_copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	img_copy_region.imageSubresource.mipLevel = 0;
	img_copy_region.imageSubresource.baseArrayLayer = 0;
	img_copy_region.imageSubresource.layerCount = 1;
	img_copy_region.imageOffset = {0, 0, 0};
	img_copy_region.imageExtent = {
		img_width,
		img_height,
		1
	};
	vkCmdCopyBufferToImage(cmd_buf.m_handle, staging_buffer.m_buffer.m_handle, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_copy_region);

	transition_image_layout_transfer_dst_to_shader_readonly(ctx, cmd_buf.m_handle, img);

	vren::vk_utils::end_single_submit_command_buffer(std::move(cmd_buf), ctx->m_transfer_queue);
}

void vren::vk_utils::transition_image_layout_undefined_to_transfer_dst(
	std::shared_ptr<vren::context> const& ctx,
	VkCommandBuffer cmd_buf,
	VkImage img
)
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

void vren::vk_utils::transition_image_layout_transfer_dst_to_shader_readonly(
	std::shared_ptr<vren::context> const& ctx,
	VkCommandBuffer cmd_buf,
	VkImage img
)
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

void vren::vk_utils::transition_image_layout_undefined_to_color_attachment(
	std::shared_ptr<vren::context> const& ctx,
	VkCommandBuffer cmd_buf,
	VkImage img
)
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
	VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	vkCmdPipelineBarrier(cmd_buf, src_stage, dst_stage, NULL, 0, nullptr, 0, nullptr, 1, &mem_barrier);
}

void vren::vk_utils::transition_color_attachment_to_shader_readonly(
	std::shared_ptr<vren::context> const& ctx,
	VkCommandBuffer cmd_buf,
	VkImage image
)
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

void vren::vk_utils::transition_image_layout_color_attachment_to_present(
	std::shared_ptr<vren::context> const& ctx,
	VkCommandBuffer cmd_buf,
	VkImage img
)
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

// --------------------------------------------------------------------------------------------------------------------------------
// Image view
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_image_view vren::vk_utils::create_image_view(
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

vren::vk_sampler vren::vk_utils::create_sampler(
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

vren::vk_utils::texture vren::vk_utils::create_texture(
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
	VkSamplerAddressMode address_mode_w
)
{
	auto img = vren::vk_utils::create_image(
		ctx,
		width, height,
		format,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	);
	vren::vk_utils::upload_image_data(ctx, img.m_image.m_handle, width, height, image_data);

	auto img_view = vren::vk_image_view(
		vren::vk_utils::create_image_view(
			ctx,
			img.m_image.m_handle,
			format,
			VK_IMAGE_ASPECT_COLOR_BIT
		)
	);

	auto sampler = vren::vk_sampler(
		vren::vk_utils::create_sampler(
			ctx,
			mag_filter,
			min_filter,
			mipmap_mode,
			address_mode_u,
			address_mode_v,
			address_mode_w
		)
	);

	return vren::vk_utils::texture(
		std::move(img),
		std::move(img_view),
		std::move(sampler)
	);
}

vren::vk_utils::texture vren::vk_utils::create_color_texture(
	std::shared_ptr<vren::context> const& ctx,
	uint8_t r,
	uint8_t g,
	uint8_t b,
	uint8_t a
)
{
	uint8_t img_data[]{
		r, g, b, a
	};

	return vren::vk_utils::create_texture(
		ctx,
		1,
		1,
		img_data,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FILTER_NEAREST, // mag filter
		VK_FILTER_NEAREST, // min filter
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT
	);
}

// --------------------------------------------------------------------------------------------------------------------------------
// custom_framebuffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::custom_framebuffer::color_buffer
vren::vk_utils::custom_framebuffer::create_color_buffer(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t width,
	uint32_t height
)
{
	auto img = vren::vk_utils::create_image(
		ctx,
		width, height,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	);

	auto img_view = vren::vk_utils::create_image_view(
		ctx,
		img.m_image.m_handle,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	return color_buffer(
		std::move(img),
		std::move(img_view)
	);
}

vren::vk_utils::custom_framebuffer::depth_buffer
vren::vk_utils::custom_framebuffer::create_depth_buffer(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t width,
	uint32_t height
)
{
	auto img = vren::vk_utils::create_image(
		ctx,
		width, height,
		VK_FORMAT_D32_SFLOAT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
	);

	auto img_view = vren::vk_utils::create_image_view(
		ctx,
		img.m_image.m_handle,
		VK_FORMAT_D32_SFLOAT,
		VK_IMAGE_ASPECT_DEPTH_BIT
	);

	return depth_buffer(
		std::move(img),
		std::move(img_view)
	);
}

vren::vk_framebuffer
vren::vk_utils::custom_framebuffer::_create_framebuffer(
	std::shared_ptr<vren::context> const& ctx,
	uint32_t width,
	uint32_t height
)
{
	VkImageView attachments[]{
		m_color_buffer.m_image_view.m_handle,
		m_depth_buffer.m_image_view.m_handle
	};

	VkFramebufferCreateInfo fb_info{};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.renderPass = m_render_pass->m_handle;
	fb_info.attachmentCount = std::size(attachments);
	fb_info.pAttachments = attachments;
	fb_info.width = width;
	fb_info.height = height;
	fb_info.layers = 1;

	VkFramebuffer fb;
	vren::vk_utils::check(vkCreateFramebuffer(ctx->m_device, &fb_info, nullptr, &fb));
	return vren::vk_framebuffer(ctx, fb);
}

vren::vk_utils::custom_framebuffer::custom_framebuffer(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<vren::vk_render_pass> const& render_pass,
	uint32_t width,
	uint32_t height
) :
	m_render_pass(render_pass),
	m_color_buffer(create_color_buffer(ctx, width, height)),
	m_depth_buffer(create_depth_buffer(ctx, width, height)),
	m_framebuffer(_create_framebuffer(ctx, width, height))
{}
