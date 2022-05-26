#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <vren/pipeline/debug_renderer.hpp>
#include <vren/model/clusterized_model.hpp>

namespace vren_demo
{
	class clusterized_model_debugger
	{
	public:
		void write_debug_info_for_meshlet_geometry(
			vren::clusterized_model const& model,
			vren::debug_renderer_draw_buffer& draw_buffer
		);

		void write_debug_info_for_meshlet_bounds(
			vren::clusterized_model const& model,
			vren::debug_renderer_draw_buffer& draw_buffer
		);

		void write_debug_info_for_projected_sphere_bounds(
			vren::clusterized_model const& model,
			vren::camera const& camera,
			vren::debug_renderer_draw_buffer& draw_buffer
		);
	};
}
