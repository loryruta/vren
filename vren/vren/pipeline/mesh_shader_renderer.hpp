#pragma once

#include "Camera.hpp"
#include "gbuffer.hpp"
#include "mesh_shader_draw_pass.hpp"
#include "model/clusterized_model_draw_buffer.hpp"
#include "render_target.hpp"
#include "vk_api/buffer/buffer.hpp"
#include "vk_api/command_graph/render_graph.hpp"

namespace vren
{
    class context;

    // ------------------------------------------------------------------------------------------------
    // Mesh Shader Renderer
    // ------------------------------------------------------------------------------------------------

    class mesh_shader_renderer
    {
    private:
        vren::context const* m_context;
        vren::mesh_shader_draw_pass m_mesh_shader_draw_pass;

        VkBool32 m_occlusion_culling;

    public:
        explicit mesh_shader_renderer(vren::context const& context, VkBool32 occlusion_culling);

        inline bool is_occlusion_culling_enabled() const { return m_occlusion_culling; }

        vren::render_graph_t render(
            vren::render_graph_allocator& render_graph_allocator,
            glm::uvec2 const& screen,
            vren::camera_data const& camera_data,
            vren::light_array const& light_array,
            vren::clusterized_model_draw_buffer const& draw_buffer,
            vren::depth_buffer_pyramid const& depth_buffer_pyramid,
            vren::gbuffer const& gbuffer,
            vren::vk_utils::depth_buffer_t const& depth_buffer
        );
    };
} // namespace vren
