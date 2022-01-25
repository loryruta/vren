#pragma once

#include <filesystem>
#include <functional>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "vk_wrappers.hpp"

namespace vren // todo vren::vk_utils
{
	class renderer; // forward decl

	namespace vk_utils
	{
		void check(VkResult result);

		VkCommandBuffer begin_single_submit_command_buffer(vren::renderer& renderer, VkCommandPool cmd_pool);
		void end_single_submit_command_buffer(vren::renderer& renderer, VkQueue queue, VkCommandPool cmd_pool, VkCommandBuffer cmd_buf);
		void immediate_submit(vren::renderer& renderer, std::function<void(VkCommandBuffer)> submit_func);
	}

	// --------------------------------------------------------------------------------------------------------------------------------
	// Image
	// --------------------------------------------------------------------------------------------------------------------------------

	struct image
	{
		std::shared_ptr<vren::vk_image> m_image;
		std::shared_ptr<vren::vma_allocation> m_allocation;

		inline bool is_valid() const
		{
			return m_image->is_valid() && m_allocation->is_valid();
		}
	};

	void create_image(
		std::shared_ptr<vren::renderer> const& renderer,
		uint32_t width,
		uint32_t height,
		void* image_data,
		VkFormat format,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memory_properties,
		vren::image& result
	);

	// --------------------------------------------------------------------------------------------------------------------------------
	// Image view
	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_image_view create_image_view(
		std::shared_ptr<vren::renderer> const& renderer,
		VkImage image,
		VkFormat format,
		VkImageAspectFlagBits aspect
	);

	// --------------------------------------------------------------------------------------------------------------------------------
	// Sampler
	// --------------------------------------------------------------------------------------------------------------------------------

	vren::vk_sampler create_sampler(
		std::shared_ptr<vren::renderer> const& renderer,
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
		std::shared_ptr<vren::vk_image> m_image;
		std::shared_ptr<vren::vma_allocation> m_image_allocation;
		std::shared_ptr<vren::vk_image_view> m_image_view;
		std::shared_ptr<vren::vk_sampler> m_sampler;

		inline bool is_valid()
		{
			return m_image->is_valid() && m_image_allocation->is_valid() && m_image_view->is_valid() && m_sampler->is_valid();
		}
	};

	void create_texture(
		std::shared_ptr<vren::renderer> const& renderer,
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
	);

	void create_color_texture(
		std::shared_ptr<vren::renderer> const& renderer,
		uint8_t r,
		uint8_t g,
		uint8_t b,
		uint8_t a,
		vren::texture& result
	);

	void destroy_texture_if_any(
		vren::renderer& renderer,
		vren::texture& texture
	);

	//
}
