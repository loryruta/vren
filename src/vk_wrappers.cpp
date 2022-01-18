#include "vk_wrappers.hpp"

#include "renderer.hpp"

template<>
void vren::destroy_vk_handle(vren::renderer& renderer, VkImage handle)
{
	vkDestroyImage(renderer.m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(vren::renderer& renderer, VmaAllocation handle)
{
	vmaFreeMemory(renderer.m_gpu_allocator->m_allocator, handle);
}

template<>
void vren::destroy_vk_handle(vren::renderer& renderer, VkImageView handle)
{
	vkDestroyImageView(renderer.m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(vren::renderer& renderer, VkSampler handle)
{
	vkDestroySampler(renderer.m_device, handle, nullptr);
}

