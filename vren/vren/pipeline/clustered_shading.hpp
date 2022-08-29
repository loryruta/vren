#pragma once

#include "vk_helpers/shader.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/image.hpp"
#include "gbuffer.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"

namespace vren
{
    namespace clustered_shading
    {
        // ------------------------------------------------------------------------------------------------
        // find_unique_cluster_list
        // ------------------------------------------------------------------------------------------------

        class find_unique_cluster_list
        {
        private:
            vren::context const* m_context;

            vren::pipeline m_pipeline;

        public:
            find_unique_cluster_list(vren::context const& context);

            void operator()(
                uint32_t frame_idx,
                VkCommandBuffer command_buffer,
                vren::resource_container& resource_container,
                glm::uvec2 const& screen,
                vren::camera const& camera,
                vren::gbuffer const& gbuffer,
                vren::vk_utils::depth_buffer_t const& depth_buffer,
                vren::vk_utils::buffer const& cluster_key_buffer,
                vren::vk_utils::buffer const& allocation_index_buffer,
                vren::vk_utils::combined_image_view const& cluster_reference_buffer
            );
        };

        // ------------------------------------------------------------------------------------------------
        // assign_lights
        // ------------------------------------------------------------------------------------------------

        class assign_lights
        {
        private:
            vren::context const* m_context;

            vren::pipeline m_pipeline;

        public:
            assign_lights(vren::context const& context);

            void operator()(
                uint32_t frame_idx,
                VkCommandBuffer command_buffer,
                vren::resource_container& resource_container,
                glm::uvec2 const& screen,
                vren::camera const& camera,
                vren::vk_utils::buffer const& cluster_key_buffer,
                vren::vk_utils::buffer const& allocation_index_buffer,
                vren::vk_utils::buffer const& light_bvh_buffer,
                uint32_t light_bvh_root_index,
                uint32_t light_count,
                vren::vk_utils::buffer const& light_index_buffer,
                vren::vk_utils::buffer const& assigned_light_buffer
            );
        };

        // ------------------------------------------------------------------------------------------------
        // shade
        // ------------------------------------------------------------------------------------------------

        class shade
        {
        private:
            vren::context const* m_context;

            vren::pipeline m_pipeline;

        public:
            shade(vren::context const& context);

            void operator()(
                uint32_t frame_idx,
                VkCommandBuffer command_buffer,
                vren::resource_container& resource_container,
                glm::uvec2 const& screen,
                vren::camera const& camera,
                vren::gbuffer const& gbuffer,
                vren::vk_utils::depth_buffer_t const& depth_buffer,
                vren::vk_utils::combined_image_view const& cluster_reference_buffer,
                vren::vk_utils::buffer const& assigned_light_buffer,
                vren::light_array const& light_array,
                vren::material_buffer const& material_buffer,
                vren::vk_utils::combined_image_view const& output
            );
        };
    }

    // ------------------------------------------------------------------------------------------------
    // cluster_and_shade
    // ------------------------------------------------------------------------------------------------

    class cluster_and_shade
    {
    private:
        vren::context const* m_context;

    public:
        vren::clustered_shading::find_unique_cluster_list m_find_unique_cluster_list;
        vren::clustered_shading::assign_lights m_assign_lights;
        vren::clustered_shading::shade m_shade;

        vren::vk_utils::buffer m_cluster_key_buffer;
        vren::vk_utils::buffer m_allocation_index_buffer;
        vren::vk_utils::combined_image_view m_cluster_reference_buffer;
        vren::vk_utils::buffer m_assigned_light_buffer;

        cluster_and_shade(vren::context const& context);

        vren::render_graph_t operator()(
            vren::render_graph_allocator& allocator,
            glm::uvec2 const& screen,
            vren::camera const& camera,
            vren::gbuffer const& gbuffer,
            vren::vk_utils::depth_buffer_t const& depth_buffer,
            vren::vk_utils::buffer const& light_bvh_buffer,
            uint32_t light_bvh_root_index,
            uint32_t light_count,
            vren::vk_utils::buffer const& light_index_buffer,
            vren::light_array const& light_array,
            vren::material_buffer const& material_buffer,
            vren::vk_utils::combined_image_view const& output
        );
    };

}
