#pragma once

#include "model.hpp"

namespace vren::br
{
	using pre_upload_model_t = vren::model; // Intermediate model is fine for Basic Renderer's uploading

	class model_specializer
	{
	public:
		inline pre_upload_model_t specialize(vren::model const& model)
		{
			return model;
		}
	};
}
