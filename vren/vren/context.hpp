#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <volk.h>

#include "config.hpp"

namespace vren
{
    // Forward decl
    class toolbox;

    // ------------------------------------------------------------------------------------------------

    void get_supported_layers(std::vector<VkLayerProperties>& layers); // todo in utils/misc ?
    bool does_support_layers(std::vector<char const*> const& layers);

    // ------------------------------------------------------------------------------------------------
    // Context
    // ------------------------------------------------------------------------------------------------

    struct context_info
    {
        char const* m_app_name;
        uint32_t m_app_version;
        std::vector<char const*> m_layers;
        std::vector<char const*> m_extensions;
        std::vector<char const*> m_device_extensions;
    };

    class context
    {
    public:
        struct queue_families
        {
            uint32_t m_graphics_idx = -1;
            uint32_t m_compute_idx = -1;
            uint32_t m_transfer_idx = -1;
            uint32_t m_present_idx = -1;

            inline bool is_valid() const { return m_graphics_idx >= 0 && m_compute_idx >= 0 && m_transfer_idx >= 0; }
        };

    private:
        void print_nv_mesh_shader_info(VkPhysicalDeviceMeshShaderPropertiesNV const& props);

        VkInstance create_instance();
        VkDebugUtilsMessengerEXT create_debug_messenger();
        VkPhysicalDevice find_physical_device();
        vren::context::queue_families get_queue_families(VkPhysicalDevice physical_device);
        VkDevice create_logical_device();
        std::vector<VkQueue> get_queues();
        VmaAllocator create_vma_allocator();

    public:
        vren::context_info m_info;

        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debug_messenger;

        VkPhysicalDevice m_physical_device;
        VkPhysicalDeviceProperties m_physical_device_properties;
        VkPhysicalDeviceMemoryProperties m_physical_device_memory_properties;

        vren::context::queue_families m_queue_families;
        VkDevice m_device;

        std::vector<VkQueue> m_queues;
        VkQueue m_graphics_queue;
        VkQueue m_transfer_queue;
        // VkQueue m_compute_queue;

        VmaAllocator m_vma_allocator;

        std::unique_ptr<vren::toolbox> m_toolbox;

        explicit context(context_info const& ctx_info);
        context(context const& other) = delete;
        // TODO move constructor

        ~context();
    };
} // namespace vren
