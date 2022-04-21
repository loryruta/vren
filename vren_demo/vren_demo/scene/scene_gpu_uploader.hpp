#pragma once

#include <vren/renderer.hpp>

#include "intermediate_scene.hpp"

namespace vren_demo
{
	vren::basic_renderer_draw_buffer upload_scene_for_basic_renderer(
		vren::context const& context,
		vren_demo::intermediate_scene const& intermediate_scene
	);

	vren::mesh_shader_renderer_draw_buffer upload_scene_for_mesh_shader_renderer(
		vren::context const& context,
		vren_demo::intermediate_scene const& intermediate_scene
	);
}
