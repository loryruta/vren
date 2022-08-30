#pragma once

#include <vren/camera.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/image.hpp>
#include <vren/vk_helpers/shader.hpp>
#include <vren/pipeline/debug_renderer.hpp>
#include <vren/pipeline/render_graph.hpp>

namespace vren_demo
{
	class visualize_clusters
	{
	private:
		vren::context const* m_context;

		vren::pipeline m_pipeline;

	public:
		visualize_clusters(vren::context const& context);

		vren::render_graph_t operator()(
			vren::render_graph_allocator& render_graph_allocator,
			glm::uvec2 const& screen,
			uint32_t mode,
			vren::vk_utils::combined_image_view const& cluster_reference_buffer,
			vren::vk_utils::buffer const& cluster_key_buffer,
			vren::vk_utils::buffer const& assigned_light_buffer,
			vren::vk_utils::combined_image_view const& output
		);
	};

	vren::debug_renderer_draw_buffer create_debug_draw_buffer_with_demo_camera_clusters(vren::context const& context);
}
