#pragma once

#include <vren/model/msr_model_loader.hpp>
#include "vren/pipeline/debug_renderer.hpp"

namespace vren_demo
{
	void write_debug_info_for_meshlet_geometry(
		vren::msr::pre_upload_model const& model,
		std::vector<vren::debug_renderer_line>& lines
	);

	void write_debug_info_for_meshlet_bounds(
		vren::msr::pre_upload_model const& model,
		std::vector<vren::debug_renderer_sphere>& spheres
	);

	void write_debug_info_for

}
