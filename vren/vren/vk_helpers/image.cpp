#include "image.hpp"

#include <cstring>

#include "context.hpp"
#include "buffer.hpp"
#include "vk_helpers/image_layout_transitions.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Image
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::image
vren::vk_utils::create_image(
	vren::context const& ctx,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkMemoryPropertyFlags memory_properties,
	VkImageUsageFlags usage
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
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_create_info{};
	alloc_create_info.requiredFlags = memory_properties;

	VkImage image;
	VmaAllocation alloc;
	VmaAllocationInfo allocation_info;
	VREN_CHECK(vmaCreateImage(ctx.m_vma_allocator, &image_info, &alloc_create_info, &image, &alloc, &allocation_info), &ctx);

	return {
		.m_image = vren::vk_image(ctx, image),
		.m_allocation = vren::vma_allocation(ctx, alloc),
		.m_allocation_info = allocation_info
	};
}

void vren::vk_utils::upload_image_data(
	vren::context const& ctx,
    VkCommandBuffer cmd_buf,
    vren::resource_container& res_container,
    VkImage img,
    uint32_t img_width,
    uint32_t img_height,
    void* img_data
)
{
	VkDeviceSize img_size = img_width * img_height * 4; // Always assume RGBA

	auto staging_buffer = std::make_shared<vren::vk_utils::buffer>(
        vren::vk_utils::alloc_host_visible_buffer(ctx, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, img_size, false)
    );
    res_container.add_resource(staging_buffer);

	void* mapped_img_data;
	VREN_CHECK(vmaMapMemory(ctx.m_vma_allocator, staging_buffer->m_allocation.m_handle, &mapped_img_data), &ctx);
		std::memcpy(mapped_img_data, img_data, img_size);
	vmaUnmapMemory(ctx.m_vma_allocator, staging_buffer->m_allocation.m_handle);

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
	vkCmdCopyBufferToImage(cmd_buf, staging_buffer->m_buffer.m_handle, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_copy_region);
}

void vren::vk_utils::clear_color_image(VkCommandBuffer cmd_buf, VkImage img, VkClearColorValue clear_color)
{
	VkImageSubresourceRange subresource_range{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
	vkCmdClearColorImage(cmd_buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &subresource_range);
}

void vren::vk_utils::clear_depth_image(VkCommandBuffer cmd_buf, VkImage img, VkClearDepthStencilValue clear_depth_stencil)
{
	VkImageSubresourceRange subresource_range{
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
	vkCmdClearDepthStencilImage(cmd_buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil, 1, &subresource_range);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Image view
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_image_view vren::vk_utils::create_image_view(
	vren::context const& ctx,
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
	VREN_CHECK(vkCreateImageView(ctx.m_device, &image_view_info, nullptr, &image_view), &ctx);
	return vren::vk_image_view(ctx, image_view);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Sampler
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_sampler vren::vk_utils::create_sampler(
	vren::context const& ctx,
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
	VREN_CHECK(vkCreateSampler(ctx.m_device, &sampler_info, nullptr, &sampler), &ctx);
	return vren::vk_sampler(ctx, sampler);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Texture
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::texture vren::vk_utils::create_texture(
	vren::context const& ctx,
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
	auto img = vren::vk_utils::create_image(ctx, width, height, format, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    vren::vk_utils::immediate_graphics_queue_submit(ctx, [&](VkCommandBuffer cmd_buf, vren::resource_container& res_container)
    {
        vren::vk_utils::transition_image_layout_undefined_to_transfer_dst(cmd_buf, img.m_image.m_handle);

        vren::vk_utils::upload_image_data(ctx, cmd_buf, res_container, img.m_image.m_handle, width, height, image_data);

        vren::vk_utils::transition_image_layout_transfer_dst_to_shader_readonly(cmd_buf, img.m_image.m_handle);
    });

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

	return {
		.m_image = std::move(img),
		.m_image_view = std::move(img_view),
		.m_sampler = std::move(sampler)
	};
}

vren::vk_utils::texture vren::vk_utils::create_color_texture(
	vren::context const& ctx,
	uint8_t r,
	uint8_t g,
	uint8_t b,
	uint8_t a
)
{
	uint8_t img_data[]{ r, g, b, a };
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

vren::vk_framebuffer vren::vk_utils::create_framebuffer(vren::context const& context, VkRenderPass render_pass, uint32_t width, uint32_t height, std::span<VkImageView> const& attachments)
{
	VkFramebufferCreateInfo framebuffer_info{
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.renderPass = render_pass,
		.attachmentCount = (uint32_t) attachments.size(),
		.pAttachments = attachments.data(),
		.width = width,
		.height = height,
		.layers = 1
	};
	VkFramebuffer framebuffer;
	VREN_CHECK(vkCreateFramebuffer(context.m_device, &framebuffer_info, nullptr, &framebuffer), &context);
	return vren::vk_framebuffer(context, framebuffer);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Color buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::color_buffer_t vren::vk_utils::create_color_buffer(vren::context const& context, uint32_t width, uint32_t height, VkImageUsageFlags image_usage)
{
	VkFormat image_format = VREN_COLOR_BUFFER_OUTPUT_FORMAT;
	auto image = vren::vk_utils::create_image(context, width, height, image_format, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | image_usage);
	auto image_view = vren::vk_utils::create_image_view(context, image.m_image.m_handle, image_format, VK_IMAGE_ASPECT_COLOR_BIT);

	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_IMAGE, (uint64_t) image.m_image.m_handle, "color_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) image_view.m_handle, "color_buffer");

	return {
		.m_image = std::move(image),
		.m_image_view = std::move(image_view)
	};
}

// --------------------------------------------------------------------------------------------------------------------------------
// Depth buffer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::depth_buffer_t vren::vk_utils::create_depth_buffer(vren::context const& context, uint32_t width, uint32_t height, VkImageUsageFlags image_usage)
{
	VkFormat image_format = VREN_DEPTH_BUFFER_OUTPUT_FORMAT;
	auto image = vren::vk_utils::create_image(context, width, height, image_format, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | image_usage);
	auto image_view = vren::vk_utils::create_image_view(context, image.m_image.m_handle, image_format, VK_IMAGE_ASPECT_DEPTH_BIT);

	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_IMAGE, (uint64_t) image.m_image.m_handle, "depth_buffer");
	vren::vk_utils::set_object_name(context, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) image_view.m_handle, "depth_buffer");

	return {
		.m_image = std::move(image),
		.m_image_view = std::move(image_view)
	};
}
