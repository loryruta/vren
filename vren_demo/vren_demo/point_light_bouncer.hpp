#pragma once

#include <vren/light.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/shader.hpp>

namespace vren_demo
{
    class point_light_bouncer
    {
    private:
        vren::context const* m_context;
        vren::vk_utils::pipeline m_pipeline;

    public:
        point_light_bouncer(vren::context const& context);

    private:
        vren::vk_utils::pipeline create_pipeline();

        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& point_light_buffer,
            vren::vk_utils::buffer const& point_lights_direction_buffer
        );

    public:
        void bounce(
            uint32_t frame_idx,
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& point_light_buffer,
            vren::vk_utils::buffer const& point_lights_direction_buffer,
            glm::vec3 const& aabb_min,
            glm::vec3 const& aabb_max,
            float speed,
            float dt
        );
    };
}
