#pragma once

#include <memory>
#include <stdexcept>
#include <iostream>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace vren
{
	class renderer; // forward decl

	template<typename T>
	void destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, T handle);

	template<typename T>
	class vk_handle_wrapper
	{
	private:
		std::shared_ptr<vren::renderer> m_renderer;

	public:
		T m_handle;

		explicit vk_handle_wrapper(std::shared_ptr<vren::renderer> const& renderer, T handle) :
			m_renderer(renderer),
			m_handle(handle)
		{}

		vk_handle_wrapper(vren::vk_handle_wrapper<T> const& other) = delete;

		vk_handle_wrapper(vren::vk_handle_wrapper<T>&& other) noexcept :
			m_renderer(other.m_renderer)
		{
			*this = std::move(other);
		}

		~vk_handle_wrapper()
		{
			if (m_handle != VK_NULL_HANDLE)
			{
				vren::destroy_vk_handle<T>(m_renderer, m_handle);

				m_handle = VK_NULL_HANDLE;
			}
		}

		vren::vk_handle_wrapper<T>& operator=(vren::vk_handle_wrapper<T> const& other) = delete;

		vren::vk_handle_wrapper<T>& operator=(vren::vk_handle_wrapper<T>&& other) noexcept
		{
			//m_renderer = other.m_renderer;
			m_handle = other.m_handle;

			other.m_handle = VK_NULL_HANDLE;

			return *this;
		}

		bool is_valid()
		{
			return m_handle != VK_NULL_HANDLE;
		}
	};

	// wrapped vulkan handles
	using vk_image       = vren::vk_handle_wrapper<VkImage>;
	using vk_image_view  = vren::vk_handle_wrapper<VkImageView>;
	using vk_sampler     = vren::vk_handle_wrapper<VkSampler>;
	using vma_allocation = vren::vk_handle_wrapper<VmaAllocation>;
}
