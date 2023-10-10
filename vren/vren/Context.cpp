#include "Context.hpp"

#include <iostream>
#include <utility>

#include "Toolbox.hpp"
#include "base/misc_utils.hpp"
#include "vk_api/utils.hpp"

using namespace vren;
using namespace std::string_literals;

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    VkDebugUtilsMessengerCallbackDataEXT const* data,
    void* user_data
)
{
    fmt::color message_color = fmt::color::dark_cyan;
    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        message_color = fmt::color::yellow;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        message_color = fmt::color::red;
        break;
    default:
        break;
    }

    VREN_PRINT("[validation_layer] {}\n", fmt::format(fmt::fg(message_color), data->pMessage));

    return false;
}

std::vector<char const*> const Context::k_required_layers = {
    "VK_LAYER_KHRONOS_validation",
};

std::vector<char const*> const Context::k_required_extensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
};

std::vector<char const*> const Context::k_preferred_extensions = {};

std::vector<char const*> const Context::k_required_device_extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,    VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
    VK_KHR_8BIT_STORAGE_EXTENSION_NAME, VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
};

std::vector<char const*> const Context::k_preferred_device_extensions = {VK_NV_MESH_SHADER_EXTENSION_NAME};

Context::Context(Context::UserOptions user_options) :
    m_user_options(std::move(user_options))
{
    init_instance();
    init_debug_messenger();
    init_physical_device();
    init_logical_device();
    init_vma_allocator();

    m_toolbox = std::make_unique<Toolbox>();
}

Context::~Context()
{
    m_toolbox.reset();

    vmaDestroyAllocator(m_vma_allocator);

    m_device.reset();

    vkDestroyDebugUtilsMessengerEXT(m_instance->handle(), m_debug_messenger, nullptr);
    m_instance.reset();
}

Context& Context::init(Context::UserOptions user_options)
{
    if (s_instance)
        throw std::runtime_error("Context already initialized");
    s_instance = std::unique_ptr<Context>(new Context(std::move(user_options)));
    return *s_instance;
}

std::vector<char const*> Context::required_layers() const
{
    std::vector<char const*> layers = m_user_options.m_required_layers;
    return concat_vector(layers, k_required_layers);
}

std::vector<char const*> Context::required_extensions() const
{
    std::vector<char const*> extensions = m_user_options.m_required_extensions;
    return concat_vector(extensions, k_required_extensions);
}

std::vector<char const*> Context::preferred_extensions() const
{
    std::vector<char const*> extensions = m_user_options.m_preferred_extensions;
    return concat_vector(extensions, k_preferred_extensions);
}

std::vector<char const*> Context::required_device_extensions() const
{
    std::vector<char const*> device_extensions = m_user_options.m_required_device_extensions;
    return concat_vector(device_extensions, k_required_device_extensions);
}

std::vector<char const*> Context::preferred_device_extensions() const
{
    std::vector<char const*> device_extensions = m_user_options.m_preferred_device_extensions;
    return concat_vector(device_extensions, k_preferred_device_extensions);
}

/*
void Context::print_nv_mesh_shader_info(VkPhysicalDeviceMeshShaderPropertiesNV const& props)
{
    printf("----------------------------------------------------------------\n");
    printf("NV_mesh_shader\n");
    printf("----------------------------------------------------------------\n");

    printf("maxDrawMeshTasksCount: %d\n", props.maxDrawMeshTasksCount);
    printf("maxTaskWorkGroupInvocations: %d\n", props.maxTaskWorkGroupInvocations);
    printf("maxTaskWorkGroupSize: (%d, %d, %d)\n", props.maxTaskWorkGroupSize[0], props.maxTaskWorkGroupSize[1], props.maxTaskWorkGroupSize[2]);
    printf("maxTaskTotalMemorySize: %d\n", props.maxTaskTotalMemorySize);
    printf("maxTaskOutputCount: %d\n", props.maxTaskOutputCount);
    printf("maxMeshWorkGroupInvocations: %d\n", props.maxMeshWorkGroupInvocations);
    printf("maxMeshWorkGroupSize: (%d, %d, %d)\n", props.maxMeshWorkGroupSize[0], props.maxMeshWorkGroupSize[1], props.maxMeshWorkGroupSize[2]);
    printf("maxMeshTotalMemorySize: %d\n", props.maxMeshTotalMemorySize);
    printf("maxMeshOutputVertices: %d\n", props.maxMeshOutputVertices);
    printf("maxMeshOutputPrimitives: %d\n", props.maxMeshOutputPrimitives);
    printf("maxMeshMultiviewViewCount: %d\n", props.maxMeshMultiviewViewCount);
    printf("meshOutputPerVertexGranularity: %d\n", props.meshOutputPerVertexGranularity);
    printf("meshOutputPerPrimitiveGranularity: %d\n", props.meshOutputPerPrimitiveGranularity);
}
 */

VkDebugUtilsMessengerCreateInfoEXT create_debug_messenger_create_info()
{
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info{};
    debug_messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_info.pNext = nullptr;
    debug_messenger_info.flags = NULL;
    debug_messenger_info.messageSeverity =
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    debug_messenger_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_info.pfnUserCallback = debug_messenger_callback;
    debug_messenger_info.pUserData = nullptr;
    return debug_messenger_info;
}

void Context::init_instance()
{
    VREN_CHECK(volkInitialize());

    // Layers
    std::vector<char const*> layers = required_layers();
    int unsupported_layer_idx = Instance::support_layers(layers);
    if (unsupported_layer_idx < 0)
        throw std::runtime_error("Unsupported required layer: "s + layers.at(unsupported_layer_idx));

    // Extensions
    std::vector<char const*> extensions = required_extensions();
    int unsupported_extension_idx = Instance::support_extensions(extensions);
    if (unsupported_extension_idx >= 0)
        throw std::runtime_error("Unsupported required extension: "s + extensions.at(unsupported_extension_idx));

    for (char const* extension : preferred_extensions())
    {
        if (Instance::support_extension(extension))
            extensions.push_back(extension);
    }

    // Debug utils
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_info = create_debug_messenger_create_info();
    debug_messenger_info.pNext = nullptr;

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = m_user_options.m_app_name.c_str();
    app_info.applicationVersion = m_user_options.m_app_version;
    app_info.pEngineName = VREN_NAME;
    app_info.engineVersion = VREN_VERSION;
    app_info.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext = &debug_messenger_info;
    instance_info.flags = NULL;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledLayerCount = (uint32_t) layers.size();
    instance_info.ppEnabledLayerNames = layers.data();
    instance_info.enabledExtensionCount = (uint32_t) extensions.size();
    instance_info.ppEnabledExtensionNames = extensions.data();

    VkInstance instance_handle;
    VREN_CHECK(vkCreateInstance(&instance_info, nullptr, &instance_handle));
    m_instance = std::make_unique<Instance>(instance_handle);

    volkLoadInstance(m_instance->handle());
}

void Context::init_debug_messenger()
{
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = create_debug_messenger_create_info();
    VREN_CHECK(vkCreateDebugUtilsMessengerEXT(m_instance->handle(), &debug_messenger_create_info, nullptr, &m_debug_messenger));
}

void Context::init_physical_device()
{
    std::vector<char const*> required_extensions = required_device_extensions();

    for (PhysicalDevice& physical_device : m_instance->physical_devices())
    {
        // Has a queue family supporting graphics, compute and transfer
        int queue_family_idx = physical_device.find_queue_family(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT, NULL);
        if (queue_family_idx < 0)
            continue;

        // Supports at least all the required device extensions
        if (physical_device.support_device_extensions(required_extensions) >= 0)
            continue;

        if (!physical_device.properties().limits.timestampComputeAndGraphics)
            continue;

        m_physical_device = physical_device;
        m_queue_family_idx = queue_family_idx;
        break;
    }

    throw std::runtime_error("Couldn't find a physical device satisfying minimum requirements");
}

void Context::init_logical_device()
{
    // Queues
    int num_queues = 2;
    float queue_priorities[] = {1.0f, 1.0f};
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = m_queue_family_idx;
    queue_info.queueCount = num_queues;
    queue_info.pQueuePriorities = queue_priorities;

    // Features
    VkPhysicalDevice16BitStorageFeatures khr_16bit_storage_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES,
        .pNext = nullptr,
        .storageBuffer16BitAccess = true,
        .uniformAndStorageBuffer16BitAccess = true,
        .storagePushConstant16 = true,
        .storageInputOutput16 = false,
    };

    VkPhysicalDeviceMeshShaderFeaturesNV mesh_shader_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV, .pNext = &khr_16bit_storage_features, .taskShader = true, .meshShader = true};

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = &mesh_shader_features,
        .extendedDynamicState = true,
    };

    VkPhysicalDeviceVulkan12Features vulkan_12_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &extended_dynamic_state_features,
        .drawIndirectCount = true,
        .storageBuffer8BitAccess = true,
        .uniformAndStorageBuffer8BitAccess = true,
        .storagePushConstant8 = true,
        .descriptorBindingPartiallyBound = true,
        .descriptorBindingVariableDescriptorCount = true,
        .runtimeDescriptorArray = true,
        .samplerFilterMinmax = true,
        .separateDepthStencilLayouts = true,
    };

    VkPhysicalDeviceVulkan13Features vulkan_13_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &vulkan_12_features,
        .dynamicRendering = true,
    };

    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan_13_features,
        .features =
            {
                .fillModeNonSolid = VK_TRUE,
            },
    };

    // Device extensions
    std::vector<char const*> device_extensions = required_device_extensions();
    int unsupported_device_extension_idx = m_physical_device.support_device_extensions(device_extensions);
    if (unsupported_device_extension_idx >= 0)
        throw std::runtime_error("Unsupported required device extension: "s + device_extensions.at(unsupported_device_extension_idx));

    for (char const* device_extension : preferred_device_extensions())
    {
        if (m_physical_device.support_device_extension(device_extension))
            device_extensions.push_back(device_extension);
    }

    // Device
    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = &features2;
    device_info.flags = NULL;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = nullptr;
    device_info.enabledExtensionCount = (uint32_t) device_extensions.size();
    device_info.ppEnabledExtensionNames = device_extensions.data();
    device_info.pEnabledFeatures = nullptr;

    VkDevice handle;
    VREN_CHECK(vkCreateDevice(m_physical_device.handle(), &device_info, nullptr, &handle));
    m_device = std::make_unique<Device>(handle, 2);
}

void Context::init_vma_allocator()
{
    VmaAllocatorCreateInfo vma_allocator_info{};
    vma_allocator_info.physicalDevice = m_physical_device.handle();
    vma_allocator_info.device = m_device->handle();
    vma_allocator_info.preferredLargeHeapBlockSize = 0; // Default is 256MB
    vma_allocator_info.instance = m_instance->handle();
    vma_allocator_info.vulkanApiVersion = VK_API_VERSION_1_3;
    VREN_CHECK(vmaCreateAllocator(&vma_allocator_info, &m_vma_allocator));
}
