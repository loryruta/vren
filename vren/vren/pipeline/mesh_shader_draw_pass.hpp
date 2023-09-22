#pragma once

#include <memory>
#include <vector>

#include "vk_mem_alloc.h"
#include "volk.h"

#include "base/resource_container.hpp"
#include "depth_buffer_pyramid.hpp"
#include "gpu_repr.hpp"
#include "light.hpp"
#include "model/clusterized_model_draw_buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    // ------------------------------------------------------------------------------------------------
    // Mesh shader draw pass
    // ------------------------------------------------------------------------------------------------

    class mesh_shader_draw_pass
    {
    private:
        vren::context const* m_context;
        vren::pipeline m_pipeline;

    public:
        mesh_shader_draw_pass(vren::context const& context, VkBool32 occlusion_culling);

    private:
        vren::pipeline create_graphics_pipeline(VkBool32 occlusion_culling);

    public:
        void render(
            uint32_t frame_idx,
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::camera_data const& camera_data,
            vren::clusterized_model_draw_buffer const& draw_buffer,
            vren::light_array const& light_array,
            vren::depth_buffer_pyramid const& depth_buffer_pyramid
        );
    };
} // namespace vren
