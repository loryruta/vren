#pragma once

#include <memory>
#include <stdexcept>
#include <iostream>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include "context.hpp"

#define VREN_DEFINE_VK_RAII(_raii_t, _t, _dtor_lambda) \
    namespace vren { \
        using _raii_t = vren::vk_handle_wrapper<_t>; \
    } \
	\
	template<> \
	inline void vren::destroy_vk_handle(vren::context const& ctx, _t handle) { \
		_dtor_lambda(ctx, handle); \
	};

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// VK handle wrapper (lifetime guard)
	// ------------------------------------------------------------------------------------------------

	template<typename T>
	void destroy_vk_handle(vren::context const& ctx, T handle);

	template<typename _t>
	class vk_handle_wrapper
	{
	private:
		vren::context const* m_context;

	public:
		_t m_handle;

		explicit vk_handle_wrapper(vren::context const& ctx, _t handle) :
			m_context(&ctx),
			m_handle(handle)
		{}

		vk_handle_wrapper(vren::vk_handle_wrapper<_t> const& other) = delete;
		vk_handle_wrapper(
			vren::vk_handle_wrapper<_t>&& other
		) noexcept :
			m_context(other.m_context),
			m_handle(std::move(other.m_handle))
		{
			other.m_handle = VK_NULL_HANDLE;
		}

		~vk_handle_wrapper()
		{
			if (m_handle != VK_NULL_HANDLE)
			{
				std::cout << "Destroying " << typeid(_t).name() << std::endl;

				vren::destroy_vk_handle<_t>(*m_context, m_handle);
				m_handle = VK_NULL_HANDLE;
			}
		}

		vren::vk_handle_wrapper<_t>& operator=(vren::vk_handle_wrapper<_t>&& other) noexcept
		{
			m_context = other.m_context;
			m_handle = other.m_handle;

			other.m_handle = VK_NULL_HANDLE;

			return *this;
		}
	};
}

// --------------------------------------------------------------------------------------------------------------------------------

/* VkImage */
VREN_DEFINE_VK_RAII(vk_image, VkImage, [](vren::context const& ctx, VkImage handle) {
	vkDestroyImage(ctx.m_device, handle, nullptr);
});

/* VkImageView */
VREN_DEFINE_VK_RAII(vk_image_view, VkImageView, [](vren::context const& ctx, VkImageView handle) {
	vkDestroyImageView(ctx.m_device, handle, nullptr);
});

/* VkSampler */
VREN_DEFINE_VK_RAII(vk_sampler, VkSampler, [](vren::context const& ctx, VkSampler handle) {
	vkDestroySampler(ctx.m_device, handle, nullptr);
});

/* VkFramebuffer */
VREN_DEFINE_VK_RAII(vk_framebuffer, VkFramebuffer, [](vren::context const& ctx, VkFramebuffer handle) {
	vkDestroyFramebuffer(ctx.m_device, handle, nullptr);
});

/* VkRenderPass */
VREN_DEFINE_VK_RAII(vk_render_pass, VkRenderPass, [](vren::context const& ctx, VkRenderPass handle) {
	vkDestroyRenderPass(ctx.m_device, handle, nullptr);
});

/* VkBuffer */
VREN_DEFINE_VK_RAII(vk_buffer, VkBuffer, [](vren::context const& ctx, VkBuffer handle) {
	vkDestroyBuffer(ctx.m_device, handle, nullptr);
});

/* VkSemaphore */
VREN_DEFINE_VK_RAII(vk_semaphore, VkSemaphore, [](vren::context const& ctx, VkSemaphore handle) {
	vkDestroySemaphore(ctx.m_device, handle, nullptr);
});

/* VkFence */
VREN_DEFINE_VK_RAII(vk_fence, VkFence, [](vren::context const& ctx, VkFence handle) {
	vkDestroyFence(ctx.m_device, handle, nullptr);
});

/* VkDescriptorSetLayout */
VREN_DEFINE_VK_RAII(vk_descriptor_set_layout, VkDescriptorSetLayout, [](vren::context const& ctx, VkDescriptorSetLayout handle) {
	vkDestroyDescriptorSetLayout(ctx.m_device, handle, nullptr);
});

/* VkQueryPool */
VREN_DEFINE_VK_RAII(vk_query_pool, VkQueryPool, [](vren::context const& ctx, VkQueryPool handle) {
	vkDestroyQueryPool(ctx.m_device, handle, nullptr);
});

/* VkShaderModule */
VREN_DEFINE_VK_RAII(vk_shader_module, VkShaderModule, [](vren::context const& ctx, VkShaderModule handle) {
	vkDestroyShaderModule(ctx.m_device, handle, nullptr);
});

/* VkPipeline */
VREN_DEFINE_VK_RAII(vk_pipeline, VkPipeline, [](vren::context const& ctx, VkPipeline handle) {
	vkDestroyPipeline(ctx.m_device, handle, nullptr);
});

/* VkPipelineLayout */
VREN_DEFINE_VK_RAII(vk_pipeline_layout, VkPipelineLayout, [](vren::context const& ctx, VkPipelineLayout handle) {
	vkDestroyPipelineLayout(ctx.m_device, handle, nullptr);
});

/* VkCommandPool */
VREN_DEFINE_VK_RAII(vk_command_pool, VkCommandPool, [](vren::context const& ctx, VkCommandPool handle) {
	vkDestroyCommandPool(ctx.m_device, handle, nullptr);
});

/* VkDescriptorPool */
VREN_DEFINE_VK_RAII(vk_descriptor_pool, VkDescriptorPool, [](vren::context const& ctx, VkDescriptorPool handle) {
	vkDestroyDescriptorPool(ctx.m_device, handle, nullptr);
});

/* VkSurfaceKHR */
VREN_DEFINE_VK_RAII(vk_surface_khr, VkSurfaceKHR, [](vren::context const& ctx, VkSurfaceKHR handle) {
	vkDestroySurfaceKHR(ctx.m_instance, handle, nullptr);
});

/* VmaAllocation */
VREN_DEFINE_VK_RAII(vma_allocation, VmaAllocation, [](vren::context const& ctx, VmaAllocation handle) {
	vmaFreeMemory(ctx.m_vma_allocator, handle);
});
