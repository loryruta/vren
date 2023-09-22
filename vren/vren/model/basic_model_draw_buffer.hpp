#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/debug_utils.hpp"

namespace vren
{
    struct basic_model_draw_buffer
    {
        std::string m_name = "unnamed";

        struct mesh
        {
            uint32_t m_vertex_offset, m_vertex_count;
            uint32_t m_index_offset, m_index_count;
            uint32_t m_instance_offset, m_instance_count;
            uint32_t m_material_idx;
        };

        vren::vk_utils::buffer m_vertex_buffer;
        vren::vk_utils::buffer m_index_buffer;
        vren::vk_utils::buffer m_instance_buffer;
        std::vector<mesh> m_meshes;

        inline void set_object_names(vren::context const& context)
        {
            vren::vk_utils::set_object_name(
                context, VK_OBJECT_TYPE_BUFFER, (uint64_t) m_vertex_buffer.m_buffer.m_handle, "vertex_buffer"
            );
            vren::vk_utils::set_object_name(
                context, VK_OBJECT_TYPE_BUFFER, (uint64_t) m_index_buffer.m_buffer.m_handle, "index_buffer"
            );
            vren::vk_utils::set_object_name(
                context, VK_OBJECT_TYPE_BUFFER, (uint64_t) m_instance_buffer.m_buffer.m_handle, "instance_buffer"
            );
        }
    };
} // namespace vren
