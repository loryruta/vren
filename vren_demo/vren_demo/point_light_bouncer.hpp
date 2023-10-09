#pragma once

#include <vren/light.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/shader.hpp>
#include <vren/pipeline/debug_renderer.hpp>
#include <vren/pipeline/render_graph.hpp>

namespace vren_demo
{
    class point_light_bouncer
    {
    public:
        static const uint32_t k_workgroup_size = 1024;

    private:
        vren::context const* m_context;
        vren::pipeline m_pipeline;

    public:
        point_light_bouncer(vren::context const& context);

    public:
        void bounce(
            uint32_t frame_idx,
            VkCommandBuffer command_buffer,
            vren::ResourceContainer& resource_container,
            vren::vk_utils::buffer const& point_light_position_buffer,
            vren::vk_utils::buffer const& point_light_direction_buffer,
            uint32_t point_light_count,
            glm::vec3 const& aabb_min,
            glm::vec3 const& aabb_max,
            float speed,
            float dt
        );
    };

    class fill_point_light_debug_draw_buffer
    {
    private:
        vren::context const* m_context;
        vren::pipeline m_pipeline;

    public:
        fill_point_light_debug_draw_buffer(vren::context const& context);

        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& point_light_buffer,
            uint32_t point_light_count,
            vren::debug_renderer_draw_buffer const& debug_draw_buffer
        );

        vren::render_graph_t operator()(
            vren::render_graph_allocator& allocator,
            vren::vk_utils::buffer const& point_light_buffer,
            uint32_t point_light_count,
            vren::debug_renderer_draw_buffer const& debug_draw_buffer
        );
    };
}
