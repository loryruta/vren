#include "context.hpp"

#include <iostream>

#include "toolbox.hpp"
#include "vk_helpers/vk_ext.hpp"
#include "vk_helpers/misc.hpp"

void vren::get_supported_layers(std::vector<VkLayerProperties>& layers)
{
	uint32_t count;
	vkEnumerateInstanceLayerProperties(&count, nullptr);

	layers.resize(count);
	vkEnumerateInstanceLayerProperties(&count, layers.data());
}

bool vren::does_support_layers(std::vector<char const*> const& layers)
{
	std::vector<VkLayerProperties> supported_layers;
	get_supported_layers(supported_layers);

	for (auto to_test : layers) {
		bool is_supported = false;
		for (auto supported_layer : supported_layers) {
			if (strcmp(to_test, supported_layer.layerName) == 0) {
				is_supported = true;
				break;
			}
		}
		if (!is_supported) {
			return false;
		}
	}

	return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT msg_severity,
	VkDebugUtilsMessageTypeFlagsEXT msg_type,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void* user_data
)
{
	if ((msg_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) || (msg_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
		std::cerr << "[VK] " << data->pMessage << std::endl;
	} else {
		std::cout << "[VK] " << data->pMessage << std::endl;
	}
	return VK_FALSE;
}

// --------------------------------------------------------------------------------------------------------------------------------
// context
// --------------------------------------------------------------------------------------------------------------------------------

VkDebugUtilsMessengerCreateInfoEXT create_debug_messenger_create_info()
{
	return VkDebugUtilsMessengerCreateInfoEXT{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.flags = NULL,
		.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
		.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = debug_messenger_callback,
		.pUserData = nullptr,
	};
}

VkInstance vren::context::create_instance()
{
	auto debug_messenger_info = create_debug_messenger_create_info();

	/* App info */
	VkApplicationInfo app_info{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = m_info.m_app_name,
		.applicationVersion = m_info.m_app_version,
		.pEngineName = VREN_NAME,
		.engineVersion = VREN_VERSION,
		.apiVersion = VK_API_VERSION_1_2
	};

	/* Extensions */
	std::vector<char const*> extensions = m_info.m_extensions;
	extensions.insert(extensions.end(), {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	});

	/* Layers */
	std::vector<char const*> layers = m_info.m_layers;
	layers.insert(layers.end(), {
		"VK_LAYER_KHRONOS_validation"
	});

	/* Instance info */
	VkInstanceCreateInfo inst_info{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = &debug_messenger_info,
		.flags = NULL,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = (uint32_t) layers.size(),
		.ppEnabledLayerNames = layers.data(),
		.enabledExtensionCount = (uint32_t) extensions.size(),
		.ppEnabledExtensionNames = extensions.data(),
	};
	VkInstance instance;
	vren::vk_utils::check(vkCreateInstance(&inst_info, nullptr, &instance));
	vren::load_instance_extensions(instance);
	return instance;
}

VkDebugUtilsMessengerEXT vren::context::create_debug_messenger()
{
	auto debug_messenger_info = create_debug_messenger_create_info();

	VkDebugUtilsMessengerEXT debug_messenger;
	vren::vk_utils::check(vkCreateDebugUtilsMessengerEXT(m_instance, &debug_messenger_info, nullptr, &debug_messenger));

	return debug_messenger;
}

VkPhysicalDevice vren::context::find_physical_device()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_instance, &count, nullptr);

	std::vector<VkPhysicalDevice> physical_devices(count);
	vkEnumeratePhysicalDevices(m_instance, &count, physical_devices.data());

	VkPhysicalDevice found = VK_NULL_HANDLE;
	for (VkPhysicalDevice physical_device : physical_devices)
	{
		if (get_queue_families(physical_device).is_valid()) {
			found = physical_device;
			break;
		}
	}

	if (found == VK_NULL_HANDLE) {
		throw std::runtime_error("Can't find a physical device that fits requirements.");
	}
	vkGetPhysicalDeviceProperties(found, &m_physical_device_properties);
	vkGetPhysicalDeviceMemoryProperties(found, &m_physical_device_memory_properties);

	if (!m_physical_device_properties.limits.timestampComputeAndGraphics) {
		throw std::runtime_error("Unsupported timestamp queries for compute and graphics queue families");
	}

	std::cout << "Physical device found:" << std::endl;
	std::cout << "- Type:           " << m_physical_device_properties.deviceType << std::endl;
	std::cout << "- Vendor ID:      " << m_physical_device_properties.vendorID << std::endl;
	std::cout << "- Device ID:      " << m_physical_device_properties.deviceID << std::endl;
	std::cout << "- Driver version: " << m_physical_device_properties.driverVersion << std::endl;
	std::cout << "- Name:           " << m_physical_device_properties.deviceName << std::endl;
	std::cout << "- API version:    " << m_physical_device_properties.apiVersion << std::endl;

	return found;
}

vren::context::queue_families vren::context::get_queue_families(VkPhysicalDevice physical_device)
{
	auto queue_families_properties = vren::vk_utils::get_queue_families_properties(physical_device);

	vren::context::queue_families queue_families;
	for (int i = 0; i < queue_families_properties.size(); i++) {
		// Graphics
		if (queue_families_properties.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) { // todo possibly graphics only queue
			queue_families.m_graphics_idx = i;
		}

		// Compute
		if (queue_families_properties.at(i).queueFlags & VK_QUEUE_COMPUTE_BIT) { // todo possibly compute only queue
			queue_families.m_compute_idx = i;
		}

		// Transfer
		if (queue_families_properties.at(i).queueFlags & VK_QUEUE_TRANSFER_BIT) { // todo possibly transfer only queue
			queue_families.m_transfer_idx = i;
		}

		if (queue_families.is_valid()) {
			break;
		}
	}

	return queue_families;
}

VkDevice vren::context::create_logical_device()
{
	uint32_t queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);

	/* Queue infos */
	std::vector<VkDeviceQueueCreateInfo> queue_infos(queue_families_count);
	for (int i = 0; i < queue_families_count; i++)
	{
		float priority = 1.0f;
		queue_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_infos[i].queueFamilyIndex = i;
		queue_infos[i].queueCount = 1;
		queue_infos[i].pQueuePriorities = &priority;
	}

	/* Extensions */
	std::vector<char const*> dev_ext = m_info.m_device_extensions;
	dev_ext.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

	/* Feature: extended dynamic state */
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_feature{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
		.pNext = nullptr,
		.extendedDynamicState = VK_TRUE,
	};

	VkPhysicalDeviceFeatures features{
		.fillModeNonSolid = VK_TRUE,
	};

	/* Device */
	VkDeviceCreateInfo dev_info{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &extended_dynamic_state_feature,
		.flags = NULL,
		.queueCreateInfoCount = (uint32_t) queue_infos.size(),
		.pQueueCreateInfos = queue_infos.data(),
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t) dev_ext.size(),
		.ppEnabledExtensionNames = dev_ext.data(),
		.pEnabledFeatures = &features
	};
	VkDevice device;
	vren::vk_utils::check(vkCreateDevice(m_physical_device, &dev_info, nullptr, &device));
	vren::load_device_extensions(device);
	return device;
}

std::vector<VkQueue> vren::context::get_queues()
{
	uint32_t queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);

	std::vector<VkQueue> queues(queue_families_count);
	for (uint32_t i = 0; i < queue_families_count; i++)
	{
		vkGetDeviceQueue(m_device, i, 0, &queues.at(i)); // Gets a queue from every queue family
	}
	return queues;
}

VmaAllocator vren::context::create_vma_allocator()
{
	VmaAllocatorCreateInfo allocator_info{
		.flags = NULL,
		.physicalDevice = m_physical_device,
		.device = m_device,
		.preferredLargeHeapBlockSize = 0, // default is 256MB
		.pAllocationCallbacks = nullptr,
		.pDeviceMemoryCallbacks = nullptr,
		.pHeapSizeLimit = nullptr,
		.pVulkanFunctions = nullptr,
		.instance = m_instance,
		.vulkanApiVersion = VK_API_VERSION_1_2,
		.pTypeExternalMemoryHandleTypes = nullptr,
	};

	VmaAllocator allocator;
	vren::vk_utils::check(vmaCreateAllocator(&allocator_info, &allocator));
	return allocator;
}

vren::context::context(context_info const& info) :
	m_info(info),

	m_instance(create_instance()),
	m_debug_messenger(create_debug_messenger()),
	m_physical_device(find_physical_device()),
	m_queue_families(get_queue_families(m_physical_device)),
	m_device(create_logical_device()),
	m_queues(get_queues()),
	m_graphics_queue(m_queues.at(m_queue_families.m_graphics_idx)),
	m_transfer_queue(m_queues.at(m_queue_families.m_transfer_idx)),
	//m_compute_queue(m_queues.at(m_queue_families.m_compute_idx)),
	m_vma_allocator(create_vma_allocator())
{
	m_toolbox = std::make_unique<vren::toolbox>(*this);
	m_toolbox->lazy_initialize();
}

vren::context::~context()
{;
	m_toolbox.reset();

	vmaDestroyAllocator(m_vma_allocator);

	vkDestroyDevice(m_device, nullptr);

	vkDestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}
