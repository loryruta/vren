#pragma once

#include "volk.h"

#include "vren/vk_api/image/image.hpp"

namespace vren
{
    struct render_target
    {
        vren::vk_utils::color_buffer_t const* m_color_buffer;
        vren::vk_utils::depth_buffer_t const* m_depth_buffer;
        VkRect2D m_render_area;
        VkViewport m_viewport;
        VkRect2D m_scissor;

        inline static render_target cover(
            uint32_t image_width,
            uint32_t image_height,
            vren::vk_utils::color_buffer_t const& color_buffer,
            vren::vk_utils::depth_buffer_t const& depth_buffer
        )
        {
            return {
                .m_color_buffer = &color_buffer,
                .m_depth_buffer = &depth_buffer,
                .m_render_area = {.offset = {0, 0}, .extent = {image_width, image_height}},
                .m_viewport =
                    {.x = 0,
                     .y = (float) image_height,
                     .width = (float) image_width,
                     .height = -((float) image_height),
                     .minDepth = 0.0f,
                     .maxDepth = 1.0f},
                .m_scissor{.offset = {0, 0}, .extent = {0, 0}}};
        }
    };
} // namespace vren
