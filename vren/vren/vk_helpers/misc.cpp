#include "misc.hpp"

#include "context.hpp"
#include "log.hpp"
#include "pool/command_pool.hpp"
#include "toolbox.hpp"

void what_the_fuck_i_did_wrong(VkQueue queue)
{
    uint32_t checkpointCount = 0;
    vkGetQueueCheckpointDataNV(queue, &checkpointCount, 0);

    std::vector<VkCheckpointDataNV> checkpoints(checkpointCount, {VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV});
    vkGetQueueCheckpointDataNV(queue, &checkpointCount, checkpoints.data());

    for (auto& checkpoint : checkpoints)
    {
        VREN_ERROR(
            "NV CHECKPOINT: stage {:x} name {}\n",
            checkpoint.stage,
            checkpoint.pCheckpointMarker ? static_cast<const char*>(checkpoint.pCheckpointMarker) : "??"
        );
    }
}

void vren::vk_utils::check(VkResult result, vren::context const* context)
{
    if (result != VK_SUCCESS)
    {
        VREN_ERROR("Vulkan command failed: {} ({:#x})\n", "-", result);

        if (context && result == VK_ERROR_DEVICE_LOST)
        {
            VREN_ERROR("Graphics queue checkpoints:\n");
            what_the_fuck_i_did_wrong(context->m_graphics_queue);

            VREN_ERROR("Transfer queue checkpoints:\n");
            what_the_fuck_i_did_wrong(context->m_transfer_queue);
        }

        throw std::runtime_error("Vulkan command failed");
    }
}

void vren::vk_utils::check(VkResult result)
{
    vren::vk_utils::check(result, nullptr);
}

vren::vk_semaphore vren::vk_utils::create_semaphore(vren::context const& ctx)
{
    VkSemaphoreCreateInfo sem_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = NULL};

    VkSemaphore sem;
    VREN_CHECK(vkCreateSemaphore(ctx.m_device, &sem_info, nullptr, &sem), &ctx);
    return vren::vk_semaphore(ctx, sem);
}

vren::vk_fence vren::vk_utils::create_fence(vren::context const& ctx, bool signaled)
{
    VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : (VkFenceCreateFlags) NULL};

    VkFence fence;
    VREN_CHECK(vkCreateFence(ctx.m_device, &fence_info, nullptr, &fence), &ctx);
    return vren::vk_fence(ctx, fence);
}

void vren::vk_utils::immediate_submit(
    vren::context const& ctx, vren::command_pool& cmd_pool, VkQueue queue, record_commands_func_t const& record_func
)
{
    auto cmd_buf = cmd_pool.acquire();

    vren::resource_container res_container;

    VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr};
    VREN_CHECK(vkBeginCommandBuffer(cmd_buf.m_handle, &begin_info), &ctx);

    record_func(cmd_buf.m_handle, res_container);

    VREN_CHECK(vkEndCommandBuffer(cmd_buf.m_handle), &ctx);

    auto fence = vren::vk_utils::create_fence(ctx);

    VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = NULL,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf.m_handle,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr};
    VREN_CHECK(vkQueueSubmit(queue, 1, &submit_info, fence.m_handle), &ctx);

    VREN_CHECK(vkWaitForFences(ctx.m_device, 1, &fence.m_handle, VK_TRUE, UINT64_MAX), &ctx);
}

void vren::vk_utils::immediate_graphics_queue_submit(
    vren::context const& ctx, record_commands_func_t const& record_func
)
{
    vren::vk_utils::immediate_submit(ctx, ctx.m_toolbox->m_graphics_command_pool, ctx.m_graphics_queue, record_func);
}

void vren::vk_utils::immediate_transfer_queue_submit(
    vren::context const& ctx, record_commands_func_t const& record_func
)
{
    vren::vk_utils::immediate_submit(ctx, ctx.m_toolbox->m_transfer_command_pool, ctx.m_transfer_queue, record_func);
}

vren::vk_utils::surface_details
vren::vk_utils::get_surface_details(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    vren::vk_utils::surface_details surf_det{};

    // Surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surf_det.m_capabilities);

    // Surface formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

    if (format_count != 0)
    {
        surf_det.m_surface_formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, surface, &format_count, surf_det.m_surface_formats.data()
        );
    }

    // Present modes
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
        surf_det.m_present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device, surface, &present_mode_count, surf_det.m_present_modes.data()
        );
    }

    return surf_det;
}

std::vector<VkQueueFamilyProperties> vren::vk_utils::get_queue_families_properties(VkPhysicalDevice phy_dev)
{
    uint32_t queue_fam_props_cnt;
    vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &queue_fam_props_cnt, nullptr);

    std::vector<VkQueueFamilyProperties> queue_fam_props(queue_fam_props_cnt);
    vkGetPhysicalDeviceQueueFamilyProperties(phy_dev, &queue_fam_props_cnt, queue_fam_props.data());

    return queue_fam_props;
}

void vren::vk_utils::pipeline_barrier(VkCommandBuffer cmd_buf)
{
    VkMemoryBarrier memory_barrier{};
    memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

    vkCmdPipelineBarrier(
        cmd_buf,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        NULL,
        1,
        &memory_barrier,
        0,
        nullptr,
        0,
        nullptr
    );
}

void vren::vk_utils::pipeline_barrier(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& buffer)
{
    VkBufferMemoryBarrier buffer_memory_barrier{};
    buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buffer_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    buffer_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    buffer_memory_barrier.buffer = buffer.m_buffer.m_handle;
    buffer_memory_barrier.offset = 0;
    buffer_memory_barrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(
        cmd_buf,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        NULL,
        0,
        nullptr,
        1,
        &buffer_memory_barrier,
        0,
        nullptr
    );
}

void vren::vk_utils::pipeline_barrier(VkCommandBuffer cmd_buf, vren::vk_utils::image const& image)
{
    VkImageMemoryBarrier image_memory_barrier{};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL; // TODO non-specified layout without losing image data ?
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_memory_barrier.image = image.m_image.m_handle;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        cmd_buf,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        NULL,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &image_memory_barrier
    );
}

void vren::vk_utils::transit_image_layout(
    VkCommandBuffer command_buffer,
    vren::vk_utils::image const& image,
    VkImageLayout src_image_layout,
    VkImageLayout dst_image_layout
)
{
    VkImageMemoryBarrier image_memory_barrier{};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
    image_memory_barrier.oldLayout = src_image_layout;
    image_memory_barrier.newLayout = dst_image_layout;
    image_memory_barrier.image = image.m_image.m_handle;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        NULL,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &image_memory_barrier
    );
}