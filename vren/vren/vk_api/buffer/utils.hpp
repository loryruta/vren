#pragma once

#include <volk.h>

#include "Buffer.hpp"
#include "base/ResourceContainer.hpp"

namespace vren
{
    Buffer&& allocate_buffer(
        VkMemoryPropertyFlags memory_properties,
        VkBufferUsageFlags buffer_usage,
        size_t size,
        bool persistently_mapped = false
        );

    void update_buffer_deferred(
        std::shared_ptr<Buffer>& buffer,
        void const* data,
        size_t size,
        size_t offset,
        VkCommandBuffer command_buffer,
        ResourceContainer& resource_container
        );

    void update_buffer(
        Buffer& buffer,
        void const* data,
        size_t size,
        size_t offset
        );

    void copy_buffer_deferred(
        std::shared_ptr<Buffer>& src_buffer,
        std::shared_ptr<Buffer>& dst_buffer,
        size_t size,
        size_t src_offset,
        size_t dst_offset,
        VkCommandBuffer command_buffer,
        ResourceContainer& resource_container
        );

    void copy_buffer(
        Buffer& src_buffer,
        Buffer& dst_buffer,
        size_t size,
        size_t src_offset,
        size_t dst_offset
        );

    Buffer&& import_buffer(
        void const* data,
        size_t size
        );
}
