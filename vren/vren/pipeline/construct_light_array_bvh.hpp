#pragma once

#include "context.hpp"
#include "light.hpp"
#include "pipeline/render_graph.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    class construct_light_array_bvh
    {
    private:
        vren::context const* m_context;

        vren::pipeline m_discretize_light_positions_pipeline;
        vren::pipeline m_init_light_array_bvh_pipeline;

    public:
        construct_light_array_bvh(vren::context const& context);

        static VkBufferUsageFlags get_required_bvh_buffer_usage_flags();
        static size_t get_required_bvh_buffer_size(uint32_t light_count);

        static VkBufferUsageFlags get_required_light_index_buffer_usage_flags();
        static size_t get_required_light_index_buffer_size(uint32_t light_count);

        vren::render_graph_t construct(
            vren::render_graph_allocator& render_graph_allocator,
            vren::light_array const& light_array,
            vren::vk_utils::buffer const& bvh_buffer,
            size_t bvh_buffer_offset,
            vren::vk_utils::buffer const& light_index_buffer,
            size_t light_index_buffer_offset
        );
    };
}
