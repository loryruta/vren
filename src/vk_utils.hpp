#pragma once

#include <filesystem>

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

		inline bool is_valid() const
		{
			return m_handle != VK_NULL_HANDLE && m_allocation != VK_NULL_HANDLE;
		}

		inline bool operator==(vren::image const& other) const
		{
			return
				m_handle == other.m_handle &&
				m_format == other.m_format &&
				m_allocation == other.m_allocation;
		}
	};

	void create_image(vren::renderer& renderer, uint32_t width, uint32_t height, void* image_data, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_properties, vren::image& result);
	void destroy_image(vren::renderer& renderer, vren::image& result);

	struct image_view
	{
		VkImageView m_handle = VK_NULL_HANDLE;

		inline bool is_valid() const
		{
			return m_handle != VK_NULL_HANDLE;
		}

		inline bool operator==(vren::image_view const& other) const
		{
			return m_handle == other.m_handle;
		}
	};

	void create_image_view(vren::renderer& renderer, vren::image const& image, VkImageAspectFlagBits aspect, vren::image_view& result);
	void destroy_image_view_if_any(vren::renderer& renderer, vren::image_view& result);

	// ------------------------------------------------------------------------------------------------ Texture

	struct texture
	{
		vren::image m_image;
		vren::image_view m_image_view;

		VkSampler m_sampler_handle = VK_NULL_HANDLE;

		inline bool operator==(vren::texture const& other) const
		{
			return
				m_sampler_handle == other.m_sampler_handle &&
				m_image_view == other.m_image_view &&
				m_image == other.m_image;
		}
	};

	void create_texture(
		vren::renderer& renderer,
		uint32_t width,
		uint32_t height,
		void* image_data,
		VkFormat format,

		vren::texture& result
	);

	void destroy_texture_if_any(
		vren::renderer& renderer,
		vren::texture& texture
	);

	//

	namespace vk_utils
	{
		void check(VkResult result);
	}
}
