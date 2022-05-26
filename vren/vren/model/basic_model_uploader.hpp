#pragma once

#include "model.hpp"
#include "basic_model_draw_buffer.hpp"
#include "pipeline/basic_renderer.hpp"

namespace vren
{
	class basic_model_uploader
	{
	public:
		vren::basic_model_draw_buffer upload(vren::context const& context, vren::model const& model);
	};
}
