#pragma once

#include <functional>
#include <memory>

#include "base/ResourceContainer.hpp"
#include "config.hpp"
#include "vk_api/buffer/Buffer.hpp"
#include "vk_api/image/utils.hpp"
#include "vk_raii.hpp"

#define VREN_CHECK(_vk_result) vren::check(_vk_result)

namespace vren
{
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

    vren::surface_details get_surface_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

    vren::vk_query_pool create_timestamp_query_pool(vren::context const& ctx, uint32_t query_count);

    void write_buffer_descriptor(
        VkDescriptorSet descriptor_set,
        uint32_t binding,
        VkBuffer buffer,
        size_t range,
        size_t offset
        );

    void write_storage_image_descriptor(
        VkDescriptorSet descriptor_set,
        uint32_t binding,
        VkImageView image_view,
        VkImageLayout image_layout
        );

    void write_combined_image_sampler_descriptor(
        VkDescriptorSet descriptor_set,
        uint32_t binding,
        VkSampler sampler,
        VkImageView image_view,
        VkImageLayout image_layout
        );

    // ------------------------------------------------------------------------------------------------
    // Pipeline barriers
    // ------------------------------------------------------------------------------------------------

    void pipeline_barrier(VkCommandBuffer command_buffer);
    void pipeline_barrier(VkCommandBuffer command_buffer, Buffer& buffer);
    void pipeline_barrier(VkCommandBuffer command_buffer, Image& image);

    void transit_image_layout(
        VkCommandBuffer command_buffer,
        vren::vk_utils::utils const& image,
        VkImageLayout src_image_layout,
        VkImageLayout dst_image_layout
    );
} // namespace vren::vk_utils
