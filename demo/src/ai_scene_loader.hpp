#pragma once

#include "renderer.hpp"

#include <filesystem>

#include <assimp/scene.h>

namespace vren_demo
{
	class ai_scene_baker
	{
	public:
		vren::renderer& m_renderer;

		ai_scene_baker(vren::renderer& renderer);
		~ai_scene_baker();

		void create_texture(aiTexture const* ai_tex, vren::texture& texture);
		void create_material_texture(
			aiScene const* ai_scene,
			aiMaterial const* ai_mat,
			char const* texture_key, unsigned int texture_type, unsigned int texture_index,
			vren::texture& result
		);

		void bake(aiScene const* ai_scene, vren::render_list& render_list);
	};
}
