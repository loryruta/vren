#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#define VREN_RAII_DECLARE_ALIAS(_alias_t, _vk_handle_t) \
    namespace vren \
    { \
        using _alias_t = vren::vk_handle_wrapper<_vk_handle_t>; \
    }

namespace vren
{
    template <typename _vk_handle_t>
    class vk_handle_wrapper
    {
    private:
        _vk_handle_t m_handle;

    public:
        explicit vk_handle_wrapper(_vk_handle_t handle) :
            m_handle(handle)
        {
        }

        vk_handle_wrapper(vk_handle_wrapper const& other) = delete;
        vk_handle_wrapper(vk_handle_wrapper&& other) noexcept :
            m_handle(other.m_handle)
        {
            other.m_handle = VK_NULL_HANDLE;
        }

        ~vk_handle_wrapper()
        {
            if (m_handle != VK_NULL_HANDLE)
                destroy_handle(m_handle);
        }

        _vk_handle_t get() const { return m_handle; }

    private:
        void destroy_handle(_vk_handle_t handle)
        {
            static_assert("Not implemented");
        }
    };
} // namespace vren

VREN_RAII_DECLARE_ALIAS(vk_instance, VkInstance);
VREN_RAII_DECLARE_ALIAS(vk_device, VkDevice);
VREN_RAII_DECLARE_ALIAS(vk_image, VkImage);
VREN_RAII_DECLARE_ALIAS(vk_image_view, VkImageView);
VREN_RAII_DECLARE_ALIAS(vk_sampler, VkSampler);
VREN_RAII_DECLARE_ALIAS(vk_framebuffer, VkFramebuffer);
VREN_RAII_DECLARE_ALIAS(vk_render_pass, VkRenderPass);
VREN_RAII_DECLARE_ALIAS(vk_buffer, VkBuffer);
VREN_RAII_DECLARE_ALIAS(vk_semaphore, VkSemaphore);
VREN_RAII_DECLARE_ALIAS(vk_fence, VkFence);
VREN_RAII_DECLARE_ALIAS(vk_descriptor_set_layout, VkDescriptorSetLayout);
VREN_RAII_DECLARE_ALIAS(vk_query_pool, VkQueryPool);
VREN_RAII_DECLARE_ALIAS(vk_shader_module, VkShaderModule);
VREN_RAII_DECLARE_ALIAS(vk_pipeline_layout, VkPipelineLayout);
VREN_RAII_DECLARE_ALIAS(vk_pipeline, VkPipeline);
VREN_RAII_DECLARE_ALIAS(vk_command_pool, VkCommandPool);
VREN_RAII_DECLARE_ALIAS(vk_descriptor_pool, VkDescriptorPool);
VREN_RAII_DECLARE_ALIAS(vk_surface_khr, VkSurfaceKHR);
VREN_RAII_DECLARE_ALIAS(vma_allocation, VmaAllocation);
