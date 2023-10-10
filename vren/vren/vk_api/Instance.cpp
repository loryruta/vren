#include "Instance.hpp"

using namespace vren;

namespace
{
    std::vector<VkLayerProperties> g_layers;
    std::vector<VkExtensionProperties> g_extensions;
} // namespace

Instance::Instance(VkInstance handle) :
    m_handle(handle)
{
}

std::span<PhysicalDevice> Instance::physical_devices()
{
    if (m_physical_devices.empty())
    {
        uint32_t num_physical_devices = 0;
        vkEnumeratePhysicalDevices(m_handle.get(), &num_physical_devices, nullptr);

        std::vector<VkPhysicalDevice> handles(num_physical_devices);
        vkEnumeratePhysicalDevices(m_handle.get(), &num_physical_devices, handles.data());

        std::vector<PhysicalDevice> physical_devices{};
        for (VkPhysicalDevice handle : handles)
            m_physical_devices.emplace_back(handle);
    }
    return m_physical_devices;
}

void Instance::clear_cached_data()
{
    m_physical_devices.clear();
}

std::vector<VkLayerProperties> const& Instance::layers()
{
    if (g_layers.empty())
    {
        uint32_t num_layers;
        vkEnumerateInstanceLayerProperties(&num_layers, nullptr);

        g_layers.resize(num_layers);
        vkEnumerateInstanceLayerProperties(&num_layers, g_layers.data());
    }
    return g_layers;
}

bool Instance::support_layer(char const* layer)
{
    for (VkLayerProperties const& supported_layer : g_layers)
    {
        if (std::strcmp(supported_layer.layerName, layer) == 0)
            return true;
    }
    return false;
}

int Instance::support_layers(std::span<char const*> const layers)
{
    // todo optimize
    for (int i = 0; i < layers.size(); i++)
    {
        if (!support_layer(layers[i]))
            return i;
    }
    return -1;
}

std::vector<VkExtensionProperties> const& Instance::extensions()
{
    if (g_extensions.empty())
    {
        uint32_t count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

        g_extensions.resize(count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, g_extensions.data());
    }
    return g_extensions;
}

bool Instance::support_extension(char const* extension)
{
    // todo optimize
    for (VkExtensionProperties const& supported_extension : g_extensions)
    {
        if (std::strcmp(supported_extension.extensionName, extension) == 0)
            return true;
    }
    return false;
}

int Instance::support_extensions(std::span<char const*> const extensions)
{
    for (int i = 0; i < extensions.size(); i++)
    {
        if (!support_layer(extensions[i]))
            return i;
    }
    return -1;
}

void Instance::clear_static_cached_data()
{
    g_layers.clear();
    g_extensions.clear();
}
