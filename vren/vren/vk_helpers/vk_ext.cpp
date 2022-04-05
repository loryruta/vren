#include "vk_ext.hpp"

PFN_vkCreateDebugUtilsMessengerEXT  g_vkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT g_vkDestroyDebugUtilsMessengerEXT;

PFN_vkCmdSetPrimitiveTopologyEXT g_vkCmdSetPrimitiveTopologyEXT;

// --------------------------------------------------------------------------------------------------------------------------------

void vren::load_instance_extensions(VkInstance instance)
{
	g_vkCreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)  vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	g_vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
}

void vren::load_device_extensions(VkDevice device)
{
	g_vkCmdSetPrimitiveTopologyEXT = (PFN_vkCmdSetPrimitiveTopologyEXT) vkGetDeviceProcAddr(device, "vkCmdSetPrimitiveTopologyEXT");
}

// --------------------------------------------------------------------------------------------------------------------------------

VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	return g_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	return g_vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
}

void vkCmdSetPrimitiveTopologyEXT(VkCommandBuffer commandBuffer, VkPrimitiveTopology primitiveTopology)
{
	return g_vkCmdSetPrimitiveTopologyEXT(commandBuffer, primitiveTopology);
}
