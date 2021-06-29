#pragma once

#include "renderer.hpp"

#include <filesystem>

namespace vren_demo
{
	struct material{};

	struct scene
	{
		std::vector<vren_demo::material> m_materials;
		std::vector<vren::vertex> m_vertices;
		std::vector<vren::vertex_index_t> m_indices;
	};

	bool load_scene(std::filesystem::path const& in_file, vren_demo::scene& result);
}
