#pragma once

#include <cstdint>
#include <vector>

#include "gpu_repr.hpp"
#include "mesh.hpp"
#include "msr_model_specializer.hpp"
#include "pipeline/msr_renderer.hpp"

namespace vren::msr
{
	class model_uploader
	{
	public:
		vren::msr::draw_buffer upload(vren::context const& context, pre_upload_model const& pre_upload_model);
	};
}
