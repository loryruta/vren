#pragma once


#include <memory>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>


namespace vren
{
	class renderer; // forward decl

	template<typename T>
	void destroy_vk_handle(vren::renderer& renderer, T handle);

	template<typename T>
	class vk_handle_wrapper
	{
	private:
		vren::renderer& m_renderer;

	public:
		T m_handle;

		explicit vk_handle_wrapper(vren::renderer& renderer, T handle) :
			m_renderer(renderer),
			m_handle(handle)
		{
		}

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

	// unique ref
	template<typename _t>
	using unq = std::unique_ptr<_t>;

	template <class _t, class... _args_t>
	vren::unq<_t> make_unq(_args_t&&... args)
	{
		return std::make_unique<_t>(std::forward<_args_t>(args)...); // todo why std::forward ?
	}

	// ref counting
	template<typename _t>
	using rc = std::shared_ptr<_t>;

	template <class _t, class... _args_t>
	vren::rc<_t> make_rc(_args_t&&... args)
	{
		return std::make_shared<_t>(std::forward<_args_t>(args)...);
	}

	// wrapped vulkan handles
	using vk_image      = vren::vk_handle_wrapper<VkImage>;
	using vk_image_view = vren::vk_handle_wrapper<VkImageView>;
	using vk_sampler    = vren::vk_handle_wrapper<VkSampler>;

	using vma_allocation = vren::vk_handle_wrapper<VmaAllocation>;
}
