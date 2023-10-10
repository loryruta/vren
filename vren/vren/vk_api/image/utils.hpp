#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <span>

#include "vk_mem_alloc.h"
#include "volk.h"

#include "base/ResourceContainer.hpp"
#include "pool/CommandPool.hpp"
#include "vk_api/vk_raii.hpp"

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

    struct Image
    {
    private:
        vk_image m_image;
        vma_allocation m_allocation;

    public:
        explicit Image(VkImage image, VmaAllocation allocation);
        ~Image() = default;

        VmaAllocationInfo vma_allocation_info() const;
    };

    vren::vk_utils::utils
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
        vren::ResourceContainer& res_container,
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
        VkImage image,
        VkFormat format,
        VkImageAspectFlags image_aspect_flags
    );

    struct CombinedImageView
    {
        vren::vk_utils::utils m_image;
        vren::vk_image_view m_image_view;

        VkImage image() const { return m_image.m_image; }
        VkImageView image_view() const { return m_image_view; }
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
        vren::vk_utils::utils m_image;
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

    class Texture
    {
    private:
        std::shared_ptr<Image> m_image;
        std::shared_ptr<vk_image_view> m_image_view;
        std::shared_ptr<vk_sampler> m_sampler;

    public:
        explicit Texture(
            std::shared_ptr<Image> image,
            std::shared_ptr<vk_image_view> image_view,
            std::shared_ptr<vk_sampler> sampler
            );
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
