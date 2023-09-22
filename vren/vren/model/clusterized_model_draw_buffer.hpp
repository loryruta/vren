#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/debug_utils.hpp"

namespace vren
{
    struct clusterized_model_draw_buffer
    {
        std::string m_name = "unnamed";

        vren::vk_utils::buffer m_vertex_buffer;
        vren::vk_utils::buffer m_meshlet_vertex_buffer;
        vren::vk_utils::buffer m_meshlet_triangle_buffer;
        vren::vk_utils::buffer m_meshlet_buffer;
        vren::vk_utils::buffer m_instanced_meshlet_buffer;
        size_t m_instanced_meshlet_count;
        vren::vk_utils::buffer m_instance_buffer;

        inline void set_object_names(vren::context const& context)
        {
            vren::vk_utils::set_object_name(
                context, VK_OBJECT_TYPE_BUFFER, (uint64_t) m_vertex_buffer.m_buffer.m_handle, "vertex_buffer"
            );
            vren::vk_utils::set_object_name(
                context,
                VK_OBJECT_TYPE_BUFFER,
                (uint64_t) m_meshlet_vertex_buffer.m_buffer.m_handle,
                "meshlet_vertex_buffer"
            );
            vren::vk_utils::set_object_name(
                context,
                VK_OBJECT_TYPE_BUFFER,
                (uint64_t) m_meshlet_triangle_buffer.m_buffer.m_handle,
                "meshlet_triangle_buffer"
            );
            vren::vk_utils::set_object_name(
                context, VK_OBJECT_TYPE_BUFFER, (uint64_t) m_meshlet_buffer.m_buffer.m_handle, "meshlet_buffer"
            );
            vren::vk_utils::set_object_name(
                context,
                VK_OBJECT_TYPE_BUFFER,
                (uint64_t) m_instanced_meshlet_buffer.m_buffer.m_handle,
                "instanced_meshlet_buffer"
            );
            vren::vk_utils::set_object_name(
                context, VK_OBJECT_TYPE_BUFFER, (uint64_t) m_instance_buffer.m_buffer.m_handle, "instance_buffer"
            );
        }
    };
} // namespace vren
