#include "Context.hpp"
#include "wrappers/HandleDeleter.hpp"
#include <volk.h>

using namespace vren;

template <> void HandleDeleter<VkInstance>::delete_handle(VkInstance handle)
{
    vkDestroyInstance(handle, nullptr);
}

template <> void HandleDeleter<VkDevice>::delete_handle(VkDevice handle)
{
    vkDestroyDevice(handle, nullptr);
}

template <> void HandleDeleter<VkImage>::delete_handle(VkImage handle)
{
    Context& context = Context::get();
    vkDestroyImage(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkImageView>::delete_handle(VkImageView handle)
{
    Context& context = Context::get();
    vkDestroyImageView(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkSampler>::delete_handle(VkSampler handle)
{
    Context& context = Context::get();
    vkDestroySampler(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkFramebuffer>::delete_handle(VkFramebuffer handle)
{
    Context& context = Context::get();
    vkDestroyFramebuffer(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkRenderPass>::delete_handle(VkRenderPass handle)
{
    Context& context = Context::get();
    vkDestroyRenderPass(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkBuffer>::delete_handle(VkBuffer handle)
{
    Context& context = Context::get();
    vkDestroyBuffer(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkSemaphore>::delete_handle(VkSemaphore handle)
{
    Context& context = Context::get();
    vkDestroySemaphore(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkFence>::delete_handle(VkFence handle)
{
    Context& context = Context::get();
    vkDestroyFence(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkDescriptorSetLayout>::delete_handle(VkDescriptorSetLayout handle)
{
    Context& context = Context::get();
    vkDestroyDescriptorSetLayout(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkQueryPool>::delete_handle(VkQueryPool handle)
{
    Context& context = Context::get();
    vkDestroyQueryPool(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkShaderModule>::delete_handle(VkShaderModule handle)
{
    Context& context = Context::get();
    vkDestroyShaderModule(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkPipeline>::delete_handle(VkPipeline handle)
{
    Context& context = Context::get();
    vkDestroyPipeline(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkPipelineLayout>::delete_handle(VkPipelineLayout handle)
{
    Context& context = Context::get();
    vkDestroyPipelineLayout(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkCommandPool>::delete_handle(VkCommandPool handle)
{
    Context& context = Context::get();
    vkDestroyCommandPool(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkDescriptorPool>::delete_handle(VkDescriptorPool handle)
{
    Context& context = Context::get();
    vkDestroyDescriptorPool(context.device().handle(), handle, nullptr);
}

template <> void HandleDeleter<VkSurfaceKHR>::delete_handle(VkSurfaceKHR handle)
{
    Context& context = Context::get();
    vkDestroySurfaceKHR(context.instance().handle(), handle, nullptr);
}

template <> void HandleDeleter<VmaAllocation>::delete_handle(VmaAllocation handle)
{
    Context& context = Context::get();
    vmaFreeMemory(context.vma_allocator(), handle);
}
