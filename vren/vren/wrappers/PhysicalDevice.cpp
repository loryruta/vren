#include "PhysicalDevice.hpp"

#include "Context.hpp"

using namespace vren;

PhysicalDevice::PhysicalDevice(VkPhysicalDevice handle) :
    m_handle(handle)
{
}

std::vector<VkExtensionProperties> const& PhysicalDevice::device_extensions()
{
    if (m_extensions.empty())
    {
        uint32_t num_device_extensions;
        vkEnumerateDeviceExtensionProperties(m_handle, nullptr, &num_device_extensions, nullptr);

        m_extensions.resize(num_device_extensions);
        vkEnumerateDeviceExtensionProperties(m_handle, nullptr, &num_device_extensions, m_extensions.data());
    }
    return m_extensions;
}

bool PhysicalDevice::support_device_extension(char const* device_extension)
{
    // todo optimize
    for (VkExtensionProperties const& supported_extension : device_extensions())
    {
        if (std::strcmp(supported_extension.extensionName, device_extension) == 0)
            return true;
    }
    return false;
}

int PhysicalDevice::support_device_extensions(std::span<char const*> device_extensions)
{
    for (int i = 0; i < device_extensions.size(); i++)
    {
        if (!support_device_extension(device_extensions[i]))
            return i;
    }
    return -1;
}

std::vector<VkQueueFamilyProperties> const& PhysicalDevice::queue_families()
{
    if (m_queue_families.empty())
    {
        uint32_t num_queue_families;
        vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &num_queue_families, nullptr);

        std::vector<VkQueueFamilyProperties> queue_family_properties(num_queue_families);
        vkGetPhysicalDeviceQueueFamilyProperties(m_handle, &num_queue_families, queue_family_properties.data());
    }
    return m_queue_families;
}

int PhysicalDevice::find_queue_family(VkQueueFlags have_flags, VkQueueFlags not_have_flags)
{
    for (int i = 0; i < queue_families().size(); i++)
    {
        bool match = true;
        VkQueueFlags queue_flags = queue_families().at(i).queueFlags;
        match &= (queue_flags & have_flags) == have_flags;
        match &= (~queue_flags & not_have_flags) == not_have_flags;
        if (match)
            return i;
    }
    return -1;
}

VkPhysicalDeviceProperties const& PhysicalDevice::properties()
{
    if (!m_properties)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_handle, &properties);
        m_properties = properties;
    }
    return *m_properties;
}

VkPhysicalDeviceMemoryProperties const& PhysicalDevice::memory_properties()
{
    if (!m_memory_properties)
    {
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(m_handle, &memory_properties);
        m_memory_properties = memory_properties;
    }
    return *m_memory_properties;
}

void PhysicalDevice::clear_cached_data()
{
    m_extensions.clear();
    m_queue_families.clear();
    m_properties.reset();
    m_memory_properties.reset();
}
