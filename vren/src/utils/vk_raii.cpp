#include "vk_raii.hpp"

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkImage handle)
{
	vkDestroyImage(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkImageView handle)
{
	vkDestroyImageView(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkSampler handle)
{
	vkDestroySampler(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkFramebuffer handle)
{
	vkDestroyFramebuffer(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkRenderPass handle)
{
    return vkDestroyRenderPass(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkBuffer handle)
{
	vkDestroyBuffer(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkSemaphore handle)
{
	vkDestroySemaphore(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkFence handle)
{
	vkDestroyFence(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VkDescriptorSetLayout handle)
{
	vkDestroyDescriptorSetLayout(ctx->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::context> const& ctx, VmaAllocation handle)
{
	vmaFreeMemory(ctx->m_vma_allocator, handle);
}