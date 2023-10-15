#pragma once

#include "base/ResourceContainer.hpp"

namespace vren
{
    // ------------------------------------------------------------------------------------------------
    // Image
    // ------------------------------------------------------------------------------------------------

    class Image
    {
    private:
        vk_image m_image;
        vma_allocation m_allocation;

    public:
        explicit Image(VkImage image, VmaAllocation allocation);
        ~Image() = default;

        VkImage image() const { return m_image.get(); }
        VmaAllocation allocation() const { return m_allocation.get(); }

        VmaAllocationInfo vma_allocation_info() const;
    };

    Image create_image(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkMemoryPropertyFlags memory_properties,
        VkImageUsageFlags usage
        );

    /// Expects the image to be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    void record_upload_image_data(
        VkImage img,
        uint32_t img_width,
        uint32_t img_height,
        void* img_data,
        VkCommandBuffer command_buffer,
        ResourceContainer& resource_container
        );

    void upload_image_data(
        VkImage image,
        uint32_t image_width,
        uint32_t image_height,
        void* image_data
        );

    void clear_color_image(VkCommandBuffer command_buffer, VkImage image, VkClearColorValue clear_color);
    void clear_depth_image(VkCommandBuffer command_buffer, VkImage image, VkClearDepthStencilValue clear_depth_stencil);

    // ------------------------------------------------------------------------------------------------
    // Image view
    // ------------------------------------------------------------------------------------------------

    /// A RAII wrapper that ties together an image and its image view.
    class CombinedImageView
    {
    private:
        std::shared_ptr<Image> m_image;
        vk_image_view m_image_view;

    public:
        explicit CombinedImageView(std::shared_ptr<Image> image, VkImageView image_view);
        ~CombinedImageView() = default;

        VkImage image() const { return m_image->image(); }
        VkImageView image_view() const { return m_image_view.get(); }
    };

    CombinedImageView create_image_view(std::shared_ptr<Image> image, VkFormat format, VkImageAspectFlags image_aspect_flags);

    // ------------------------------------------------------------------------------------------------
    // Sampler
    // ------------------------------------------------------------------------------------------------

    vk_sampler create_sampler(
        VkFilter mag_filter,
        VkFilter min_filter,
        VkSamplerMipmapMode mipmap_mode,
        VkSamplerAddressMode address_mode_u,
        VkSamplerAddressMode address_mode_v,
        VkSamplerAddressMode address_mode_w
    );
}