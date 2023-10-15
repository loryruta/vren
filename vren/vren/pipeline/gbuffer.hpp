#pragma once

#include "vk_api/command_graph/render_graph.hpp"
#include "vk_api/debug_utils.hpp"
#include "vk_api/image/utils.hpp"
#include "vk_api/misc_utils.hpp"

namespace vren
{
    class gbuffer
    {
    private:
        vren::context const* m_context;

    public:
        uint32_t m_width;
        uint32_t m_height;

        vren::vk_utils::color_buffer_t m_normal_buffer;
        vren::vk_utils::color_buffer_t m_texcoord_buffer;
        vren::vk_utils::color_buffer_t m_material_index_buffer;

        vren::vk_sampler m_sampler;

        gbuffer(vren::context const& context, uint32_t width, uint32_t height);

        void add_render_graph_node_resources(
            vren::render_graph_node& node, VkImageLayout image_layout, VkAccessFlags access_flags
        ) const;
        void write_descriptor_set(
            VkDescriptorSet descriptor_set, vren::vk_utils::combined_image_view const& depth_buffer
        ) const;
    };

    vren::render_graph_t clear_gbuffer(vren::render_graph_allocator& allocator, vren::gbuffer const& gbuffer);
} // namespace vren
