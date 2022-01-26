#include "vk_wrappers.hpp"

#include "renderer.hpp"

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VkImage handle)
{
	vkDestroyImage(renderer->m_device, handle, nullptr);
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

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VkBuffer handle)
{
	vkDestroyBuffer(renderer->m_device, handle, nullptr);
}

template<>
void vren::destroy_vk_handle(std::shared_ptr<vren::renderer> const& renderer, VmaAllocation handle)
{
	vmaFreeMemory(renderer->m_vma_allocator, handle);
}
