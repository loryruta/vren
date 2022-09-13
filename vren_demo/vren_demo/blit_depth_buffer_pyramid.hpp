#pragma once

#include <vren/context.hpp>
#include <vren/pipeline/render_graph.hpp>
#include <vren/pipeline/depth_buffer_pyramid.hpp>
#include <vren/vk_helpers/image.hpp>
#include <vren/vk_helpers/shader.hpp>

namespace vren_demo
{
    class blit_depth_buffer_pyramid
    {
	private:
		vren::context const* m_context;

		vren::pipeline m_pipeline;
		vren::vk_sampler m_sampler;

	public:
		blit_depth_buffer_pyramid(vren::context const& context);

    public:
		vren::render_graph_t operator()(
			vren::render_graph_allocator& allocator,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid,
			uint32_t level,
			vren::vk_utils::combined_image_view const& color_buffer,
			uint32_t width,
			uint32_t height
		);
    };
}
