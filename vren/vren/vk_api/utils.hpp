#pragma once

#include <functional>
#include <memory>

#include "base/ResourceContainer.hpp"
#include "pool/CommandPool.hpp"
#include "vk_api/buffer/Buffer.hpp"
#include "vk_api/image/utils.hpp"
#include "vk_raii.hpp"

#define VREN_CHECK(...) vren::vk_utils::check(__VA_ARGS__)

namespace vren
{
    // Forward decl
    class context;
} // namespace vren

namespace vren::vk_utils
{
    void check(VkResult result, vren::context const* context);
    void check(VkResult result);

    vren::vk_semaphore create_semaphore(vren::context const& ctx);

    vren::vk_fence create_fence(vren::context const& ctx, bool signaled = false);

    // ------------------------------------------------------------------------------------------------
    // Immediate submission
    // ------------------------------------------------------------------------------------------------

    using record_commands_func_t =
        std::function<void(VkCommandBuffer cmd_buf, vren::ResourceContainer& res_container)>;

    void immediate_submit(
        vren::context const& ctx, vren::CommandPool& cmd_pool, VkQueue queue, record_commands_func_t const& record_func
    );

    void immediate_graphics_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);
    void immediate_transfer_queue_submit(vren::context const& ctx, record_commands_func_t const& record_func);

    // ------------------------------------------------------------------------------------------------
    // Surface details
    // ------------------------------------------------------------------------------------------------

    struct surface_details
    {
        VkSurfaceCapabilitiesKHR m_capabilities;
        std::vector<VkPresentModeKHR> m_present_modes;
        std::vector<VkSurfaceFormatKHR> m_surface_formats;
    };

    vren::vk_utils::surface_details get_surface_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    vren::vk_query_pool create_timestamp_query_pool(vren::context const& ctx, uint32_t query_count);

    // ------------------------------------------------------------------------------------------------

    inline void write_buffer_descriptor(
        vren::context const& context,
        VkDescriptorSet descriptor_set,
        uint32_t binding,
        VkBuffer buffer,
        size_t range,
        size_t offset
    )
    {
        assert(offset % VREN_MIN_STORAGE_BUFFER_OFFSET_ALIGNMENT == 0);

        VkDescriptorBufferInfo buffer_info{
            .buffer = buffer,
            .offset = offset,
            .range = range > 0 ? range : 1, // TODO Better way to handle the case where range is 0?
        };
        VkWriteDescriptorSet descriptor_set_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptor_set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr};
        vkUpdateDescriptorSets(context.m_device, 1, &descriptor_set_write, 0, nullptr);
    }

    inline void write_storage_image_descriptor(
        vren::context const& context,
        VkDescriptorSet descriptor_set,
        uint32_t binding,
        VkImageView image_view,
        VkImageLayout image_layout
    )
    {
        VkDescriptorImageInfo image_info{
            .sampler = nullptr,
            .imageView = image_view,
            .imageLayout = image_layout,
        };
        VkWriteDescriptorSet descriptor_set_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptor_set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr};
        vkUpdateDescriptorSets(context.m_device, 1, &descriptor_set_write, 0, nullptr);
    }

    inline void write_combined_image_sampler_descriptor(
        vren::context const& context,
        VkDescriptorSet descriptor_set,
        uint32_t binding,
        VkSampler sampler,
        VkImageView image_view,
        VkImageLayout image_layout
    )
    {
        VkDescriptorImageInfo image_info{
            .sampler = sampler,
            .imageView = image_view,
            .imageLayout = image_layout,
        };
        VkWriteDescriptorSet descriptor_set_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = descriptor_set,
            .dstBinding = binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr};
        vkUpdateDescriptorSets(context.m_device, 1, &descriptor_set_write, 0, nullptr);
    }

    // ------------------------------------------------------------------------------------------------
    // Pipeline barriers
    // ------------------------------------------------------------------------------------------------

    void pipeline_barrier(VkCommandBuffer cmd_buf);
    void pipeline_barrier(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& buffer);
    void pipeline_barrier(VkCommandBuffer cmd_buf, vren::vk_utils::utils const& image);

    void transit_image_layout(
        VkCommandBuffer command_buffer,
        vren::vk_utils::utils const& image,
        VkImageLayout src_image_layout,
        VkImageLayout dst_image_layout
    );
} // namespace vren::vk_utils
