#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <span>

#include <vk_mem_alloc.h>
#include <volk.h>

#include "base/resource_container.hpp"
#include "pool/CommandPool.hpp"
#include "vk_raii.hpp"

namespace vren
{
    // Forward decl
    class context;
} // namespace vren

namespace vren::vk_utils
{
    // ------------------------------------------------------------------------------------------------
    // Image
    // ------------------------------------------------------------------------------------------------

    struct image
    {
        vren::vk_image m_image;
        vren::vma_allocation m_allocation;
        VmaAllocationInfo m_allocation_info;
    };

    vren::vk_utils::image create_image(
        vren::context const& ctx,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkMemoryPropertyFlags memory_properties,
        VkImageUsageFlags usage
    );

    /** Expects the image to be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL */
    void upload_image_data(
        vren::context const& ctx,
        VkCommandBuffer cmd_buf,
        vren::resource_container& res_container,
        VkImage img,
        uint32_t img_width,
        uint32_t img_height,
        void* img_data
    );

    void clear_color_image(VkCommandBuffer cmd_buf, VkImage img, VkClearColorValue clear_color);
    void clear_depth_image(VkCommandBuffer cmd_buf, VkImage img, VkClearDepthStencilValue clear_depth_stencil);

    // ------------------------------------------------------------------------------------------------
    // Image view
    // ------------------------------------------------------------------------------------------------

    vren::vk_image_view create_image_view(
        vren::context const& context, VkImage image, VkFormat format, VkImageAspectFlags image_aspect_flags
    );

    struct combined_image_view_ref // Non-owning reference to combined_image_view
    {
        VkImage m_image;
        VkImageView m_image_view;
    };

    struct combined_image_view
    {
        vren::vk_utils::image m_image;
        vren::vk_image_view m_image_view;

        inline auto get_image() const { return m_image.m_image.m_handle; }

        inline auto get_image_view() const { return m_image_view.m_handle; }

        inline vren::vk_utils::combined_image_view_ref get_ref() const
        {
            return vren::vk_utils::combined_image_view_ref{.m_image = get_image(), .m_image_view = get_image_view()};
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Sampler
    // ------------------------------------------------------------------------------------------------

    vren::vk_sampler create_sampler(
        vren::context const& ctx,
        VkFilter mag_filter,
        VkFilter min_filter,
        VkSamplerMipmapMode mipmap_mode,
        VkSamplerAddressMode address_mode_u,
        VkSamplerAddressMode address_mode_v,
        VkSamplerAddressMode address_mode_w
    );

    // ------------------------------------------------------------------------------------------------
    // Image buffer
    // ------------------------------------------------------------------------------------------------

    struct storage_image
    {
        vren::vk_utils::image m_image;
        vren::vk_image_view m_image_view;
    };

    vren::vk_utils::storage_image create_storage_image(
        vren::context const& context,
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkMemoryPropertyFlags memory_property_flags,
        VkImageUsageFlags image_usage_flags
    );

    // ------------------------------------------------------------------------------------------------
    // Texture
    // ------------------------------------------------------------------------------------------------

    struct texture
    {
        vren::vk_utils::image m_image;
        vren::vk_image_view m_image_view;
        vren::vk_sampler m_sampler;
    };

    vren::vk_utils::texture create_texture(
        vren::context const& ctx,
        uint32_t width,
        uint32_t height,
        void* image_data,
        VkFormat format,
        VkFilter mag_filter,
        VkFilter min_filter,
        VkSamplerMipmapMode mipmap_mode,
        VkSamplerAddressMode address_mode_u,
        VkSamplerAddressMode address_mode_v,
        VkSamplerAddressMode address_mode_w
    );

    vren::vk_utils::texture create_color_texture(vren::context const& ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    // ------------------------------------------------------------------------------------------------
    // Color buffer
    // ------------------------------------------------------------------------------------------------

    using color_buffer_t = vren::vk_utils::combined_image_view;

    color_buffer_t create_color_buffer(
        vren::context const& context,
        uint32_t width,
        uint32_t height,
        VkFormat image_format,
        VkMemoryPropertyFlags memory_property_flags,
        VkImageUsageFlags image_usage_flags
    );

    // ------------------------------------------------------------------------------------------------
    // Depth buffer
    // ------------------------------------------------------------------------------------------------

    using depth_buffer_t = vren::vk_utils::combined_image_view;

    depth_buffer_t
    create_depth_buffer(vren::context const& context, uint32_t width, uint32_t height, VkImageUsageFlags image_usage);

    // ------------------------------------------------------------------------------------------------
    // Framebuffer
    // ------------------------------------------------------------------------------------------------

    vren::vk_framebuffer create_framebuffer(
        vren::context const& context,
        VkRenderPass render_pass,
        uint32_t width,
        uint32_t height,
        std::span<VkImageView> const& attachments
    );
} // namespace vren::vk_utils
