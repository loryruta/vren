#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vk_mem_alloc.h>
#include <volk.h>

#include "vk_api/Device.hpp"
#include "vk_api/Instance.hpp"
#include "vk_api/PhysicalDevice.hpp"
#include "Toolbox.hpp"
#include "config.hpp"

namespace vren
{
    class Context
    {
        friend class CommandPool;
        friend CommandPool CommandPool::create();
        friend class Device;
        friend class Toolbox;

    public:
        struct UserOptions
        {
            std::string m_app_name;
            uint32_t m_app_version;
            std::vector<char const*> m_required_layers;
            std::vector<char const*> m_required_extensions;
            std::vector<char const*> m_preferred_extensions;
            std::vector<char const*> m_required_device_extensions;
            std::vector<char const*> m_preferred_device_extensions;
        };

        static std::vector<char const*> const k_required_layers;
        static std::vector<char const*> const k_required_extensions;
        static std::vector<char const*> const k_preferred_extensions;
        static std::vector<char const*> const k_required_device_extensions;
        static std::vector<char const*> const k_preferred_device_extensions;

    private:
        static std::unique_ptr<Context> s_instance;

        UserOptions m_user_options;

        std::unique_ptr<Instance> m_instance;
        VkDebugUtilsMessengerEXT m_debug_messenger;
        PhysicalDevice m_physical_device;
        int m_queue_family_idx;
        std::unique_ptr<Device> m_device;
        VmaAllocator m_vma_allocator;

        std::unique_ptr<Toolbox> m_toolbox;

        explicit Context(Context::UserOptions user_options);

    public:
        Context(Context const& other) = delete;
        ~Context();

        Instance& instance() { return *m_instance; }
        PhysicalDevice& physical_device() { return m_physical_device; };
        Device& device() { return *m_device; }
        VmaAllocator vma_allocator() { return m_vma_allocator; }

        Toolbox& toolbox() { return *m_toolbox; }

        static Context& init(Context::UserOptions user_options);
        static Context& get() { return *s_instance; }

    private:
        std::vector<char const*> required_layers() const;
        std::vector<char const*> required_extensions() const;
        std::vector<char const*> preferred_extensions() const;
        std::vector<char const*> required_device_extensions() const;
        std::vector<char const*> preferred_device_extensions() const;

        void init_instance();
        void init_debug_messenger();
        void init_physical_device();
        void init_logical_device();
        void init_vma_allocator();
    };
} // namespace vren
