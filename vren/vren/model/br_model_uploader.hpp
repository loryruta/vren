#pragma once

#include "model.hpp"
#include "br_model_specializer.hpp"
#include "pipeline/br_renderer.hpp"

namespace vren::br
{
	class model_uploader
	{
	public:
		vren::br::draw_buffer upload(vren::context const& context, pre_upload_model_t const& model);
	};
}
