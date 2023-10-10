#include "vk_raii.hpp"

#include "Context.hpp"

using namespace vren;

template <> void vk_handle_wrapper<VkInstance>::destroy_handle(VkInstance handle)
{
    vkDestroyInstance(handle, nullptr);
}

template <> void vk_handle_wrapper<VkDevice>::destroy_handle(VkDevice handle)
{
    vkDestroyDevice(handle, nullptr);
}

template <> void vk_handle_wrapper<VkImage>::destroy_handle(VkImage handle)
{
    vkDestroyImage(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkImageView>::destroy_handle(VkImageView handle)
{
    vkDestroyImageView(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkSampler>::destroy_handle(VkSampler handle)
{
    vkDestroySampler(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkFramebuffer>::destroy_handle(VkFramebuffer handle)
{
    vkDestroyFramebuffer(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkRenderPass>::destroy_handle(VkRenderPass handle)
{
    vkDestroyRenderPass(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkBuffer>::destroy_handle(VkBuffer handle)
{
    vkDestroyBuffer(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkSemaphore>::destroy_handle(VkSemaphore handle)
{
    vkDestroySemaphore(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkFence>::destroy_handle(VkFence handle)
{
    vkDestroyFence(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkDescriptorSetLayout>::destroy_handle(VkDescriptorSetLayout handle)
{
    vkDestroyDescriptorSetLayout(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkQueryPool>::destroy_handle(VkQueryPool handle)
{
    vkDestroyQueryPool(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkShaderModule>::destroy_handle(VkShaderModule handle)
{
    vkDestroyShaderModule(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkPipeline>::destroy_handle(VkPipeline handle)
{
    vkDestroyPipeline(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkPipelineLayout>::destroy_handle(VkPipelineLayout handle)
{
    vkDestroyPipelineLayout(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkCommandPool>::destroy_handle(VkCommandPool handle)
{
    vkDestroyCommandPool(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkDescriptorPool>::destroy_handle(VkDescriptorPool handle)
{
    vkDestroyDescriptorPool(Context::get().device().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VkSurfaceKHR>::destroy_handle(VkSurfaceKHR handle)
{
    vkDestroySurfaceKHR(Context::get().instance().handle(), handle, nullptr);
}

template <> void vk_handle_wrapper<VmaAllocation>::destroy_handle(VmaAllocation handle)
{
    vmaFreeMemory(Context::get().vma_allocator(), handle);
}

