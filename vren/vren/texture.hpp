#pragma once

#include <vector>

#include "vk_helpers/image.hpp"

namespace vren
{
	class texture_manager
	{
	public:
		std::vector<vren::vk_utils::texture> m_textures;
	};

}
