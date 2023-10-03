#pragma once

#include <optional>
#include <span>
#include <vector>
#include <volk.h>

namespace vren
{
    // Forward decl
    class Context;

    class PhysicalDevice
    {
    private:
        VkPhysicalDevice m_handle = VK_NULL_HANDLE;

        std::vector<VkExtensionProperties> m_extensions;
        std::vector<VkQueueFamilyProperties> m_queue_families;

        std::optional<VkPhysicalDeviceProperties> m_properties;
        std::optional<VkPhysicalDeviceMemoryProperties> m_memory_properties;

    public:
        explicit PhysicalDevice() = default;
        explicit PhysicalDevice(VkPhysicalDevice handle);
        ~PhysicalDevice() = default;

        VkPhysicalDevice handle() const { return m_handle; }

        std::vector<VkExtensionProperties> const& device_extensions();
        bool support_device_extension(char const* device_extension);
        int support_device_extensions(std::span<char const*> device_extensions);

        std::vector<VkQueueFamilyProperties> const& queue_families();

        /// Searches for the queue family that has have_flags and don't have not_have_flags.
        /// \return the index of the queue family, -1 if not found.
        int find_queue_family(VkQueueFlags have_flags, VkQueueFlags not_have_flags);

        VkPhysicalDeviceProperties const& properties();
        VkPhysicalDeviceMemoryProperties const& memory_properties();

        void clear_cached_data();
    };
} // namespace vren