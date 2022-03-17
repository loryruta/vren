#pragma once

#include <memory>
#include <filesystem>
#include <functional>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "context.hpp"
#include "vk_raii.hpp"
#include "resource_container.hpp"
#include "pooling/command_pool.hpp"

namespace vren::vk_utils
{
	// --------------------------------------------------------------------------------------------------------------------------------
	// Image
	// --------------------------------------------------------------------------------------------------------------------------------

	struct image
	{
		vren::vk_image m_image;
		vren::vma_allocation m_allocation;
	};

	vren::vk_utils::image create_image(
		std::shared_ptr<vren::context> const& ctx,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkMemoryPropertyFlags memory_properties,
		VkImageUsageFlags usage
	);

    /** Expects the image to be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL */
	void upload_image_data(
        std::shared_ptr<vren::context> const& ctx,
        VkCommandBuffer cmd_buf,
        vren::resource_container& res_container,
        VkImage img,
        uint32_t img_width,
        uint32_t img_height,
        void* img_data
    );

	// --------------------------------------------------------------------------------------------------------------------------------
	// Image view
	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_image_view create_image_view(
		std::shared_ptr<vren::context> const& ctx,
		VkImage image,
		VkFormat format,
		VkImageAspectFlagBits aspect
	);

	// --------------------------------------------------------------------------------------------------------------------------------
	// Sampler
	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_sampler create_sampler(
		std::shared_ptr<vren::context> const& ctx,
		VkFilter mag_filter,
		VkFilter min_filter,
		VkSamplerMipmapMode mipmap_mode,
		VkSamplerAddressMode address_mode_u,
		VkSamplerAddressMode address_mode_v,
		VkSamplerAddressMode address_mode_w
	);

	// --------------------------------------------------------------------------------------------------------------------------------
	// Texture
	// --------------------------------------------------------------------------------------------------------------------------------

	struct texture
	{
		vren::vk_utils::image m_image;
		vren::vk_image_view m_image_view;
		vren::vk_sampler m_sampler;

		texture(
			vren::vk_utils::image&& image,
			vren::vk_image_view&& image_view,
			vren::vk_sampler&& sampler
		) :
			m_image(std::move(image)),
			m_image_view(std::move(image_view)),
			m_sampler(std::move(sampler))
		{}
	};

	vren::vk_utils::texture create_texture(
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
	);

	vren::vk_utils::texture create_color_texture(
		std::shared_ptr<vren::context> const& ctx,
		uint8_t r,
		uint8_t g,
		uint8_t b,
		uint8_t a
	);

	// --------------------------------------------------------------------------------------------------------------------------------
	// Custom framebuffer
	// --------------------------------------------------------------------------------------------------------------------------------

	class custom_framebuffer
	{
	public:
		struct color_buffer
		{
			vren::vk_utils::image m_image;
			vren::vk_image_view m_image_view;

			color_buffer(vren::vk_utils::image&& image, vren::vk_image_view&& image_view) :
				m_image(std::move(image)),
				m_image_view(std::move(image_view))
			{}
		};

		struct depth_buffer
		{
			vren::vk_utils::image m_image;
			vren::vk_image_view m_image_view;

			depth_buffer(
				vren::vk_utils::image&& image,
				vren::vk_image_view&& image_view
			) :
				m_image(std::move(image)),
				m_image_view(std::move(image_view))
			{}
		};

		static color_buffer create_color_buffer(
			std::shared_ptr<vren::context> const& ctx,
			uint32_t width,
			uint32_t height
		);

		static depth_buffer create_depth_buffer(
			std::shared_ptr<vren::context> const& ctx,
			uint32_t width,
			uint32_t height
		);

	private:
		vren::vk_framebuffer _create_framebuffer(
			std::shared_ptr<vren::context> const& ctx,
			uint32_t width,
			uint32_t height
		);

	public:
		std::shared_ptr<vren::vk_render_pass> m_render_pass;

		color_buffer m_color_buffer;
		depth_buffer m_depth_buffer;
		vren::vk_framebuffer m_framebuffer;

		custom_framebuffer(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::vk_render_pass> const& render_pass,
			uint32_t width,
			uint32_t height
		);
	};


}
