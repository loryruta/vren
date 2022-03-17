#pragma once

#include <vulkan/vulkan.h>

namespace vren::vk_utils
{
    /** Transitions an image from an undefined layout to a transfer dst layout.
     * The transition won't wait for any memory operation to happen, but will ensure transfer writes
     * will happen only after the transition is completed. */
    void transition_image_layout_undefined_to_transfer_dst(VkCommandBuffer cmd_buf, VkImage img);

    /** Transitions an image from a transfer dst layout to a shader readonly layout.
     * The transition will wait for transfer writes to happen and will ensure shader reads (at fragment shader stage),
     * will wait for the transition. */
    void transition_image_layout_transfer_dst_to_shader_readonly(VkCommandBuffer cmd_buf, VkImage img);

    /** Transitions an image from an undefined layout to a color attachment layout.
     * The transition won't wait for any memory operation to happen, but will ensure */
    void transition_image_layout_undefined_to_color_attachment(VkCommandBuffer cmd_buf, VkImage img);
    void transition_image_layout_undefined_to_depth_stencil_attachment(VkCommandBuffer cmd_buf, VkImage img);

    /** Transitions an image from a color attachment layout to a present layout. */
    void transition_image_layout_color_attachment_to_present(VkCommandBuffer cmd_buf, VkImage img);

    /** Transitions an image from a color attachment layout to a shader render-only layout.
     * The output image is supposed to be used *only* in (and after) the next fragment shader. */
    void transition_color_attachment_to_shader_readonly(VkCommandBuffer cmd_buf, VkImage image);

    void transition_image_layout_undefined_to_shader_readonly(VkCommandBuffer cmd_buf, VkImage image);
}
