#pragma once

#include <vulkan/vulkan.h>

namespace vren
{
	void load_instance_extensions(VkInstance instance);
	void load_device_extensions(VkDevice device);
}

/* VK_EXT_debug_utils */
VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

/* VK_EXT_extended_dynamic_state */
void vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology);



