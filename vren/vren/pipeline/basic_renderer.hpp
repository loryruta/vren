#pragma once

#include <cstdint>
#include <vector>

#include "Camera.hpp"
#include "gbuffer.hpp"
#include "gpu_repr.hpp"
#include "light.hpp"
#include "model/basic_model_draw_buffer.hpp"
#include "render_graph.hpp"
#include "render_target.hpp"
#include "vk_helpers/buffer.hpp"

namespace vren
{
    // Forward decl
    class context;

    class basic_renderer
    {
    public:
        struct push_constants
        {
            glm::mat4 m_camera_view;
            glm::mat4 m_camera_projection;
            glm::uint m_material_index;
            float _pad[3]; // TODO useless
        };

    private:
        vren::context& m_context;
        vren::pipeline m_pipeline;

    public:
        explicit basic_renderer(vren::context& context);
        ~basic_renderer();

        /* Commands */

        void make_rendering_scope(
            VkCommandBuffer cmd_buf,
            glm::uvec2 const& screen,
            vren::gbuffer const& gbuffer,
            vren::vk_utils::depth_buffer_t const& depth_buffer,
            std::function<void()> const& callback
        );

        void set_viewport(VkCommandBuffer cmd_buf, float width, float height);
        void set_scissor(VkCommandBuffer cmd_buf, uint32_t width, uint32_t height);

        void set_push_constants(VkCommandBuffer cmd_buf, vren::basic_renderer::push_constants const& push_constants);

        void set_vertex_buffer(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& vertex_buffer);
        void set_index_buffer(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& index_buffer);
        void set_instance_buffer(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& instance_buffer);

        /* */

        vren::render_graph_t render(
            vren::render_graph_allocator& render_graph_allocator,
            glm::uvec2 const& screen,
            vren::Camera const& camera,
            vren::basic_model_draw_buffer const& draw_buffer,
            vren::gbuffer const& gbuffer,
            vren::vk_utils::depth_buffer_t const& depth_buffer
        );

    private:
        vren::pipeline create_graphics_pipeline();
    };
} // namespace vren
