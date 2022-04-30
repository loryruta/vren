#pragma once

#include <memory>
#include <filesystem>
#include <functional>
#include <span>

#include <volk.h>
#include <vk_mem_alloc.h>

#include "vk_raii.hpp"
#include "base/resource_container.hpp"
#include "pools/command_pool.hpp"

namespace vren
{
	// Forward decl
	class context;
}

namespace vren::vk_utils
{
	// ------------------------------------------------------------------------------------------------
	// Image
	// ------------------------------------------------------------------------------------------------

	struct image
	{
		vren::vk_image m_image;
		vren::vma_allocation m_allocation;
	};

	vren::vk_utils::image create_image(
		vren::context const& ctx,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkMemoryPropertyFlags memory_properties,
		VkImageUsageFlags usage
	);

    /** Expects the image to be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL */
	void upload_image_data(
		vren::context const& ctx,
        VkCommandBuffer cmd_buf,
        vren::resource_container& res_container,
        VkImage img,
        uint32_t img_width,
        uint32_t img_height,
        void* img_data
    );

	void clear_color_image(VkCommandBuffer cmd_buf, VkImage img, VkClearColorValue clear_color);
	void clear_depth_image(VkCommandBuffer cmd_buf, VkImage img, VkClearDepthStencilValue clear_depth_stencil);

	// ------------------------------------------------------------------------------------------------
	// Image view
	// ------------------------------------------------------------------------------------------------

	vren::vk_image_view create_image_view(
		vren::context const& ctx,
		VkImage image,
		VkFormat format,
		VkImageAspectFlagBits aspect
	);

	struct combined_image_view
	{
		vren::vk_utils::image m_image;
		vren::vk_image_view m_image_view;

		inline auto get_image() const
		{
			return m_image.m_image.m_handle;
		}

		inline auto get_image_view() const
		{
			return m_image_view.m_handle;
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Sampler
	// ------------------------------------------------------------------------------------------------

	vren::vk_sampler create_sampler(
		vren::context const& ctx,
		VkFilter mag_filter,
		VkFilter min_filter,
		VkSamplerMipmapMode mipmap_mode,
		VkSamplerAddressMode address_mode_u,
		VkSamplerAddressMode address_mode_v,
		VkSamplerAddressMode address_mode_w
	);

	// ------------------------------------------------------------------------------------------------
	// Texture
	// ------------------------------------------------------------------------------------------------

	struct texture
	{
		vren::vk_utils::image m_image;
		vren::vk_image_view m_image_view;
		vren::vk_sampler m_sampler;
	};

	vren::vk_utils::texture create_texture(
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
	);

	vren::vk_utils::texture create_color_texture(
		vren::context const& ctx,
		uint8_t r,
		uint8_t g,
		uint8_t b,
		uint8_t a
	);

	// ------------------------------------------------------------------------------------------------
	// Color buffer
	// ------------------------------------------------------------------------------------------------

	using color_buffer_t = vren::vk_utils::combined_image_view;

	color_buffer_t create_color_buffer(vren::context const& context, uint32_t width, uint32_t height, VkImageUsageFlags image_usage);

	// ------------------------------------------------------------------------------------------------
	// Depth buffer
	// ------------------------------------------------------------------------------------------------

	using depth_buffer_t = vren::vk_utils::combined_image_view;

	depth_buffer_t create_depth_buffer(vren::context const& context, uint32_t width, uint32_t height, VkImageUsageFlags image_usage);

	// ------------------------------------------------------------------------------------------------
	// Framebuffer
	// ------------------------------------------------------------------------------------------------

	vren::vk_framebuffer create_framebuffer(vren::context const& context, VkRenderPass render_pass, uint32_t width, uint32_t height, std::span<VkImageView> const& attachments);
}
