#include "renderer.hpp"

#include <iostream>

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
) {
	bool should_print = false;
	should_print |= message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	should_print |= message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	if (should_print) {
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

VkInstance vren::renderer::create_instance()
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
	if (vkCreateInstance(&instance_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the Vulkan instance.");
	}

	std::cout << "Vulkan instance initialized" << std::endl;

	return instance;
}

VkDebugUtilsMessengerEXT vren::renderer::setup_debug_messenger()
{
	VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
	populate_debug_messenger_info(debug_messenger_info);

	VkDebugUtilsMessengerEXT debug_messenger;
	if (CreateDebugUtilsMessengerEXT(m_instance, &debug_messenger_info, nullptr, &debug_messenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to setup the debug messenger.");
	}

	std::cout << "Debug messenger set up" << std::endl;

	return debug_messenger;
}

vren::renderer::queue_families vren::renderer::get_queue_families(VkPhysicalDevice physical_device)
{
	uint32_t count;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families_properties(count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, queue_families_properties.data());

	vren::renderer::queue_families queue_families;
	for (int i = 0; i < queue_families_properties.size(); i++) {
		// Graphics
		if (queue_families_properties.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queue_families.m_graphics_idx = i;
		}

		// Compute
		if (queue_families_properties.at(i).queueFlags & VK_QUEUE_COMPUTE_BIT) {
			queue_families.m_compute_idx = i;
		}

		// Transfer
		if (queue_families_properties.at(i).queueFlags & VK_QUEUE_TRANSFER_BIT) {
			queue_families.m_transfer_idx = i;
		}

		if (queue_families.is_fine()) {
			break;
		}
	}

	return queue_families;
}

VkPhysicalDevice vren::renderer::find_physical_device()
{
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(m_instance, &count, nullptr);

	std::vector<VkPhysicalDevice> physical_devices(count);
	vkEnumeratePhysicalDevices(m_instance, &count, physical_devices.data());

	VkPhysicalDevice found = VK_NULL_HANDLE;
	for (VkPhysicalDevice physical_device : physical_devices)
	{
		if (get_queue_families(physical_device).is_fine()) {
			found = physical_device;
			break;
		}
	}

	if (found == VK_NULL_HANDLE) {
		throw std::runtime_error("Can't find a physical device that fits requirements.");
	}

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(found, &properties);

	std::cout << "Physical device found:" << std::endl;
	std::cout << "- Type:           " << properties.deviceType << std::endl;
	std::cout << "- Vendor ID:      " << properties.vendorID << std::endl;
	std::cout << "- Device ID:      " << properties.deviceID << std::endl;
	std::cout << "- Driver version: " << properties.driverVersion << std::endl;
	std::cout << "- Name:           " << properties.deviceName << std::endl;
	std::cout << "- API version:    " << properties.apiVersion << std::endl;

	return found;
}

VkDevice vren::renderer::create_logical_device()
{
	uint32_t queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);

	std::vector<VkDeviceQueueCreateInfo> queue_infos(queue_families_count);
	for (int i = 0; i < queue_families_count; i++) {
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

	// Device extensions
	device_info.enabledExtensionCount = (uint32_t) m_info.m_device_extensions.size();
	device_info.ppEnabledExtensionNames = m_info.m_device_extensions.data();

	// Device layers
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = nullptr;

	VkDevice device;
	if (vkCreateDevice(m_physical_device, &device_info, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the logical device.");
	}

	std::cout << "Device created" << std::endl;

	return device;
}

VmaAllocator vren::renderer::create_allocator()
{
	VmaAllocatorCreateInfo create_info{};
	//create_info.vulkanApiVersion = VK_API_VERSION_1_2;
	create_info.instance = m_instance;
	create_info.physicalDevice = m_physical_device;
	create_info.device = m_device;

	VmaAllocator allocator;
	if (vmaCreateAllocator(&create_info, &allocator) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the VMA allocator.");
	}
	return allocator;
}

std::vector<VkQueue> vren::renderer::get_queues()
{
	uint32_t queue_families_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queue_families_count, nullptr);

	std::vector<VkQueue> queues(queue_families_count);
	for (uint32_t i = 0; i < queue_families_count; i++) {
		vkGetDeviceQueue(m_device, i, 0, &queues.at(i));
	}
	return queues;
}

VkRenderPass vren::renderer::create_render_pass()
{
	// ---------------------------------------------------------------- Attachments

	VkAttachmentDescription color_attachment{};
	color_attachment.format = VK_FORMAT_B8G8R8A8_SRGB; // Should be the format of the final target image.
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // todo

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = VK_FORMAT_D32_SFLOAT;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::initializer_list<VkAttachmentDescription> attachments = {
		color_attachment,
		depth_attachment
	};

	// ---------------------------------------------------------------- Subpasses

	std::vector<VkSubpassDescription> subpasses;
	{
		// simple_draw
		VkAttachmentReference color_attachment_ref{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;
		subpasses.push_back(subpass);
	}

	// ---------------------------------------------------------------- Dependencies

	std::vector<VkSubpassDependency> dependencies;
	{
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies.push_back(dependency);
	}

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = attachments.size();
	render_pass_info.pAttachments = attachments.begin();
	render_pass_info.subpassCount = subpasses.size();
	render_pass_info.pSubpasses = subpasses.data();
	render_pass_info.dependencyCount = dependencies.size();
	render_pass_info.pDependencies = dependencies.data();

	VkRenderPass render_pass;
	if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create the render pass.");
	}

	std::cout << "Render pass created" << std::endl;

	return render_pass;
}

void vren::renderer::create_command_pools()
{
	VkCommandPoolCreateInfo command_pool_info{};
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	// Graphics
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = m_queue_families.m_graphics_idx;
	if (vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_graphics_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics command pool.");
	}

	// Transfer
	command_pool_info.flags = NULL;
	command_pool_info.queueFamilyIndex = m_queue_families.m_transfer_idx;
	if (vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_transfer_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create transfer command pool.");
	}

	std::cout << "Command pools created" << std::endl;
}

void vren::renderer::create_white_texture()
{
	uint8_t image_data[] = {
		255, 255, 255, 255,
	};

	vren::create_texture(
		*this,
		1,
		1,
		image_data,
		VK_FORMAT_R8G8B8A8_UNORM,
		m_white_texture
	);
}

vren::material* vren::renderer::create_material()
{
	auto& material = m_materials.emplace_back(std::make_unique<vren::material>(*this));
	return material.get();
}

vren::material* vren::renderer::get_material(size_t idx)
{
	return m_materials.at(idx).get();
}

void vren::renderer::render(
	vren::frame& frame,
	vren::renderer::target const& target,
	vren::render_list const& render_list,
	vren::camera const& camera,
	std::vector<VkSemaphore> const& wait_semaphores,
	VkFence signal_fence
)
{
	// Commands re-recording
	VkCommandBuffer cmd_buf = frame.m_command_buffer;
	vkResetCommandBuffer(cmd_buf, NULL);

	VkCommandBufferBeginInfo cmd_buffer_begin_info{};
	cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(cmd_buf, &cmd_buffer_begin_info) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording to the command buffer");
	}

	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = m_render_pass;
	render_pass_begin_info.framebuffer = target.m_framebuffer;
	render_pass_begin_info.renderArea = target.m_render_area;

	std::initializer_list<VkClearValue> clear_values{
		{ .color = m_clear_color },
		{ .depthStencil = { 1.0f, 0 } }
	};
	render_pass_begin_info.clearValueCount = (uint32_t) clear_values.size();
	render_pass_begin_info.pClearValues = clear_values.begin();

	vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd_buf, 0, 1, &target.m_viewport);
	vkCmdSetScissor(cmd_buf, 0, 1, &target.m_render_area);

	m_simple_draw_pass.record_commands(frame, cmd_buf, render_list, camera);

	vkCmdEndRenderPass(cmd_buf);

	if (vkEndCommandBuffer(cmd_buf) != VK_SUCCESS) {
		throw std::runtime_error("Failed to end command buffer recording");
	}

	// Submission
	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;
	submit_info.waitSemaphoreCount = wait_semaphores.size(); // The semaphores to wait before submitting
	submit_info.pWaitSemaphores = wait_semaphores.data();
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &frame.m_render_finished_semaphore;

	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.pWaitDstStageMask = wait_stages;

	if (vkQueueSubmit(m_queues.at(m_queue_families.m_graphics_idx), 1, &submit_info, signal_fence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit command buffer.");
	}
}

vren::renderer::renderer(renderer_info& info) :
	m_info(info),

	m_instance(create_instance()),
	m_debug_messenger(setup_debug_messenger()),
	m_physical_device(find_physical_device()),
	m_allocator(create_allocator()),
	m_queue_families(get_queue_families(m_physical_device)),
	m_device(create_logical_device()),
	m_queues(get_queues()),
	m_render_pass(create_render_pass()),

	m_simple_draw_pass(*this),
	m_gpu_allocator(*this),

	m_material_descriptor_set_pool(*this)
{
	m_transfer_queue = m_queues.at(m_queue_families.m_transfer_idx);
	m_graphics_queue = m_queues.at(m_queue_families.m_graphics_idx);
	m_compute_queue  = m_queues.at(m_queue_families.m_compute_idx);

	create_command_pools();

	create_white_texture();

	m_simple_draw_pass.init();
}

vren::renderer::~renderer()
{
	// todo destroy
}
