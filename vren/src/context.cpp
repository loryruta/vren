#include "context.hpp"

#include <iostream>

#include "utils/image.hpp"
#include "descriptor_set_pool.hpp"

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

// ------------------------------------------------------------------------------------------------ renderer

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	VkDebugUtilsMessengerCallbackDataEXT const* data,
	void* user_data
)
{
	bool should_print = false;
	//should_print |= message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	should_print |= message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	should_print |= message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	if (should_print)
	{
		std::cerr << data->pMessage << std::endl;
	}

	return VK_FALSE;
}

void populate_debug_messenger_info(VkDebugUtilsMessengerCreateInfoEXT& debug_messenger_info)
{
	debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_messenger_info.pfnUserCallback = debug_messenger_callback;
}

VkInstance vren::context::create_instance()
{
	// Application info
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = m_info.m_app_name;
	app_info.applicationVersion = m_info.m_app_version;
	app_info.pEngineName = VREN_NAME;
	app_info.engineVersion = VREN_VERSION;
	app_info.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo instance_info{};
	instance_info.pNext = nullptr;
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;

	// Extensions
	std::vector<char const*> extensions = m_info.m_extensions;
	extensions.insert(extensions.end(), {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	});
	instance_info.enabledExtensionCount = (uint32_t) extensions.size();
	instance_info.ppEnabledExtensionNames = extensions.data();

	// Layers
	std::vector<char const*> layers = m_info.m_layers;
	layers.insert(layers.end(), {
		"VK_LAYER_KHRONOS_validation"
	});
	if (!vren::does_support_layers(layers)) {
		throw std::runtime_error("Layers not supported.");
	}
	instance_info.enabledLayerCount = (uint32_t) layers.size();
	instance_info.ppEnabledLayerNames = layers.data();

	// Debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
	populate_debug_messenger_info(debug_messenger_info);
	instance_info.pNext = &debug_messenger_info;

	//
	VkInstance instance;
	vren::vk_utils::check(vkCreateInstance(&instance_info, nullptr, &instance));

	std::cout << "Vulkan instance initialized" << std::endl;

	return instance;
}

VkDebugUtilsMessengerEXT vren::context::setup_debug_messenger()
{
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
	populate_debug_messenger_info(debug_messenger_info);

	VkDebugUtilsMessengerEXT debug_messenger;
	vren::vk_utils::check(CreateDebugUtilsMessengerEXT(m_instance, &debug_messenger_info, nullptr, &debug_messenger));

	std::cout << "Debug messenger set up" << std::endl;

	return debug_messenger;
}

vren::context::queue_families vren::context::get_queue_families(VkPhysicalDevice physical_device)
{
	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families_properties(count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_families_properties.data());

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

	std::cout << "Physical device found:" << std::endl;
	std::cout << "- Type:           " << m_physical_device_properties.deviceType << std::endl;
	std::cout << "- Vendor ID:      " << m_physical_device_properties.vendorID << std::endl;
	std::cout << "- Device ID:      " << m_physical_device_properties.deviceID << std::endl;
	std::cout << "- Driver version: " << m_physical_device_properties.driverVersion << std::endl;
	std::cout << "- Name:           " << m_physical_device_properties.deviceName << std::endl;
	std::cout << "- API version:    " << m_physical_device_properties.apiVersion << std::endl;

	return found;
}

VkDevice vren::context::create_logical_device()
{
	uint32_t queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);

	std::vector<VkDeviceQueueCreateInfo> queue_infos(queue_families_count);
	for (int i = 0; i < queue_families_count; i++)
	{
		float priority = 1.0f;
		queue_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_infos[i].queueFamilyIndex = i;
		queue_infos[i].queueCount = 1;
		queue_infos[i].pQueuePriorities = &priority;
	}

	VkDeviceCreateInfo device_info{};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = (uint32_t) queue_infos.size();
	device_info.pQueueCreateInfos = queue_infos.data();
	device_info.pEnabledFeatures = nullptr;
	device_info.enabledExtensionCount = (uint32_t) m_info.m_device_extensions.size(); // Device extensions
	device_info.ppEnabledExtensionNames = m_info.m_device_extensions.data();
	device_info.enabledLayerCount = 0; // Device layers
	device_info.ppEnabledLayerNames = nullptr;

	VkDevice device;
	vren::vk_utils::check(vkCreateDevice(m_physical_device, &device_info, nullptr, &device));
	return device;
}

std::vector<VkQueue> vren::context::get_queues()
{
	uint32_t queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);

	std::vector<VkQueue> queues(queue_families_count);
	for (uint32_t i = 0; i < queue_families_count; i++)
	{
		vkGetDeviceQueue(m_device, i, 0, &queues.at(i)); // Gets a queue from every queue family.
	}
	return queues;
}

void vren::context::create_vma_allocator()
{
	VmaAllocatorCreateInfo allocator_info{};
	//create_info.vulkanApiVersion = VK_API_VERSION_1_2;
	allocator_info.instance = m_instance;
	allocator_info.physicalDevice = m_physical_device;
	allocator_info.device = m_device;

	vren::vk_utils::check(vmaCreateAllocator(&allocator_info, &m_vma_allocator));
}

void vren::context::create_command_pools()
{
	VkCommandPoolCreateInfo command_pool_info{};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	// Graphics
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = m_queue_families.m_graphics_idx;
	vren::vk_utils::check(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_graphics_command_pool));

	// Transfer
	command_pool_info.flags = NULL;
	command_pool_info.queueFamilyIndex = m_queue_families.m_transfer_idx;
	vren::vk_utils::check(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_transfer_command_pool));

	// Compute TODO
}

vren::context::context(vren::context_info& info) :
	m_info(info)
{}

void vren::context::_initialize()
{
	m_instance = create_instance();
	m_debug_messenger = setup_debug_messenger();
	m_physical_device = find_physical_device();
	m_queue_families = get_queue_families(m_physical_device);

	m_device = create_logical_device();

	m_queues = get_queues();

	create_vma_allocator();

	m_transfer_queue = m_queues.at(m_queue_families.m_transfer_idx);
	m_graphics_queue = m_queues.at(m_queue_families.m_graphics_idx);
	m_compute_queue  = m_queues.at(m_queue_families.m_compute_idx);

	create_command_pools();

	// Default textures
	m_white_texture = std::make_shared<vren::texture>();
	vren::create_color_texture(shared_from_this(), 255, 255, 255, 255, *m_white_texture);

	m_black_texture = std::make_shared<vren::texture>();
	vren::create_color_texture(shared_from_this(), 0, 0, 0, 0, *m_black_texture);

	m_red_texture = std::make_shared<vren::texture>();
	vren::create_color_texture(shared_from_this(), 255, 0, 0, 255, *m_red_texture);

	m_green_texture = std::make_shared<vren::texture>();
	vren::create_color_texture(shared_from_this(), 0, 255, 0, 255, *m_green_texture);

	m_blue_texture = std::make_shared<vren::texture>();
	vren::create_color_texture(shared_from_this(), 0, 0, 255, 255, *m_blue_texture);

	m_descriptor_set_pool = std::make_unique<vren::descriptor_set_pool>(shared_from_this());
}

std::shared_ptr<vren::context> vren::context::create(vren::context_info& info)
{
	auto renderer = std::shared_ptr<vren::context>(new vren::context(info)); // renderer's initialization must be achieved in two steps because of shared_from_this()
	renderer->_initialize();
	return renderer;
}

vren::context::~context()
{;
	m_descriptor_set_pool.reset();

	vkDestroyCommandPool(m_device, m_graphics_command_pool, nullptr);
	vkDestroyCommandPool(m_device, m_transfer_command_pool, nullptr);

	vmaDestroyAllocator(m_vma_allocator);

	//m_queues.clear();

	vkDestroyDevice(m_device, nullptr);

	DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}
