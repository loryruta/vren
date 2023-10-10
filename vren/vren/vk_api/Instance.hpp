#pragma once

#include "PhysicalDevice.hpp"
#include "vk_raii.hpp"
#include <span>
#include <vector>
#include <volk.h>

namespace vren
{
    // Forward decl
    class Context;

    class Instance
    {
    private:
        vk_instance m_handle;
        std::vector<PhysicalDevice> m_physical_devices;

    public:
        explicit Instance(VkInstance handle);

        VkInstance handle() { return m_handle.get(); }

        std::span<PhysicalDevice> physical_devices();

        void clear_cached_data();

        static std::vector<VkLayerProperties> const& layers();
        static bool support_layer(char const* layer);
        static int support_layers(std::span<char const*> layers);

        static std::vector<VkExtensionProperties> const& extensions();
        static bool support_extension(char const* extension);
        static int support_extensions(std::span<char const*> extensions);

        static void clear_static_cached_data();
    };
} // namespace vren