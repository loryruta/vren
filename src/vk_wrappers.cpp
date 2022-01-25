#include "vk_wrappers.hpp"

#include "renderer.hpp"

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VkImage handle)
{
	vkDestroyImage(renderer->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VmaAllocation handle)
{
	vmaFreeMemory(renderer->m_gpu_allocator->m_allocator, handle);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VkImageView handle)
{
	vkDestroyImageView(renderer->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VkSampler handle)
{
	vkDestroySampler(renderer->m_device, handle, nullptr);
}

