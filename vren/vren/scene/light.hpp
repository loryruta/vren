#pragma once

#include <functional>
#include <queue>
#include <span>
#include <vector>

#include <glm/glm.hpp>

#include "base/base.hpp"
#include "gpu_repr.hpp"
#include "pipeline/render_graph.hpp"
#include "pool/DescriptorPool.hpp"
#include "vk_helpers/buffer.hpp"

namespace vren
{
    // Forward decl
    class context;

    class light_array
    {
    private:
        vren::context const* m_context;

    public:
        vren::vk_utils::buffer m_point_light_position_buffer; // A buffer holding only the position of the point lights,
                                                              // useful for finding min/max for clustered shading
        vren::vk_utils::buffer m_point_light_buffer;
        uint32_t m_point_light_count = 0;

        vren::vk_utils::buffer m_directional_light_buffer;
        uint32_t m_directional_light_count = 0;

        light_array(vren::context const& context);

        void add_render_graph_node_resources(vren::render_graph_node& node, VkAccessFlags access_flags) const;
        void write_descriptor_set(VkDescriptorSet descriptor_set) const;
    };
} // namespace vren
