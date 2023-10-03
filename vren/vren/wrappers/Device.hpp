#pragma once

#include "HandleDeleter.hpp"
#include <span>
#include <vector>
#include <volk.h>

namespace vren
{
    // Forward decl
    class Context;

    class Device
    {
    private:
        HandleDeleter<VkDevice> m_handle;

        int m_num_queues;
        std::vector<VkQueue> m_queues;

    public:
        explicit Device(VkDevice handle, int num_queues);

        VkDevice handle() { return m_handle.get(); }

        std::span<VkQueue> queues();

        void clear_cached_data();
    };
} // namespace vren
