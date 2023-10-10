#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "vk_api/vk_raii.hpp"

namespace vren
{
    class Buffer
    {
    private:
        vk_buffer m_buffer;
        vma_allocation m_allocation;

    public:
        explicit Buffer(VkBuffer buffer, VmaAllocation allocation);
        ~Buffer() = default;

        VkBuffer buffer() const { return m_buffer.get(); }
        VmaAllocation vma_allocation() const { return m_allocation.get(); }

        VmaAllocationInfo vma_allocation_info() const;

        void* mapped_pointer() const;
    };

} // namespace vren
