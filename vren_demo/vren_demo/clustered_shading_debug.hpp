#pragma once

#include <vren/Camera.hpp>
#include <vren/pipeline/clustered_shading.hpp>
#include <vren/pipeline/debug_renderer.hpp>
#include <vren/pipeline/render_graph.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/image.hpp>
#include <vren/vk_helpers/shader.hpp>

namespace vren_demo
{
	// ------------------------------------------------------------------------------------------------
	// show_clusters
	// ------------------------------------------------------------------------------------------------

	class show_clusters
	{
	private:
		vren::context const* m_context;

		vren::pipeline m_pipeline;

	public:
		show_clusters(vren::context const& context);

		vren::render_graph_t operator()(
			vren::render_graph_allocator& render_graph_allocator,
			glm::uvec2 const& screen,
			int32_t mode,
			vren::cluster_and_shade const& cluster_and_shade,
			vren::vk_utils::combined_image_view const& output
		);
	};

	// ------------------------------------------------------------------------------------------------
	// show_clusters_geometry
	// ------------------------------------------------------------------------------------------------

	class show_clusters_geometry
	{
	private:
		vren::context const* m_context;

		vren::pipeline m_pipeline;

	public:
		show_clusters_geometry(vren::context const& context);

		vren::debug_renderer_draw_buffer operator()(
			vren::context const& context,
			vren::cluster_and_shade const& cluster_and_shade,
			vren::Camera const& camera,
			glm::vec2 const& screen
		);
	};

	// ------------------------------------------------------------------------------------------------

	vren::debug_renderer_draw_buffer debug_camera_clusters_geometry(
		vren::context const& context,
		vren::Camera const& camera,
		glm::vec2 const& screen,
		vren::cluster_and_shade const& cluster_and_shade
	);

	vren::debug_renderer_draw_buffer create_debug_draw_buffer_with_demo_camera_clusters(vren::context const& context);
}
