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

namespace vren::vk_utils
{
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

    Texture create_texture(
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
