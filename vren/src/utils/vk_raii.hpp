#pragma once

#include <memory>
#include <stdexcept>
#include <iostream>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// vk_handle_wrapper
	// ------------------------------------------------------------------------------------------------

	template<typename T>
	void destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, T handle);

	template<typename T>
	class vk_handle_wrapper
	{
	private:
		std::shared_ptr<vren::context> m_context;

	public:
		T m_handle;

		explicit vk_handle_wrapper(std::shared_ptr<vren::context> const& ctx, T handle) :
			m_context(ctx),
			m_handle(handle)
		{}

		vk_handle_wrapper(vren::vk_handle_wrapper<T> const& other) = delete;

		vk_handle_wrapper(vren::vk_handle_wrapper<T>&& other) noexcept :
			m_context(other.m_context)
		{
			*this = std::move(other);
		}

		~vk_handle_wrapper()
		{
			if (m_handle != VK_NULL_HANDLE)
			{
				vren::destroy_vk_handle<T>(m_context, m_handle);

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

	// ------------------------------------------------------------------------------------------------

	using vk_image                 = vren::vk_handle_wrapper<VkImage>;
	using vk_image_view            = vren::vk_handle_wrapper<VkImageView>;
	using vk_sampler               = vren::vk_handle_wrapper<VkSampler>;
	using vk_framebuffer 	       = vren::vk_handle_wrapper<VkFramebuffer>;
	using vk_render_pass           = vren::vk_handle_wrapper<VkRenderPass>;
	using vk_buffer                = vren::vk_handle_wrapper<VkBuffer>;
	using vk_semaphore             = vren::vk_handle_wrapper<VkSemaphore>;
	using vk_fence                 = vren::vk_handle_wrapper<VkFence>;
	using vk_descriptor_set_layout = vren::vk_handle_wrapper<VkDescriptorSetLayout>;
	using vk_query_pool            = vren::vk_handle_wrapper<VkQueryPool>;
	using vk_shader_module         = vren::vk_handle_wrapper<VkShaderModule>;
	using vk_descriptor_pool       = vren::vk_handle_wrapper<VkDescriptorPool>;
	using vk_pipeline              = vren::vk_handle_wrapper<VkPipeline>;
	using vk_pipeline_layout       = vren::vk_handle_wrapper<VkPipelineLayout>;

	using vk_surface_khr = vren::vk_handle_wrapper<VkSurfaceKHR>;

	using vma_allocation = vren::vk_handle_wrapper<VmaAllocation>;
}
