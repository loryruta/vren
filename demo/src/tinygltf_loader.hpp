#pragma once

#include <memory>
#include <vector>

#include <tiny_gltf.h>

#include "context.hpp"
#include "render_object.hpp"

namespace vren
{
	struct tinygltf_scene
	{
		std::vector<std::shared_ptr<vren::texture>> m_textures;
		std::vector<std::shared_ptr<vren::material>> m_materials;

		std::vector<uint32_t> m_render_objects;
	};

	class tinygltf_loader
	{
	private:
		std::shared_ptr<vren::context> m_context;

		void load_textures(
			std::filesystem::path const& model_dir,
			tinygltf::Model const& gltf_model,
			vren::tinygltf_scene& result
		);

		void load_materials(
			tinygltf::Model const& gltf_model,
			vren::tinygltf_scene& result
		);

		void linearize_node_hierarchy(
			tinygltf::Model const& gltf_model,
			tinygltf::Node const& gltf_node,
			glm::mat4 transform,
			std::vector<std::vector<vren::instance_data>>& instance_data_by_mesh_idx
		);

		void load_model(
			std::filesystem::path const& model_dir,
			tinygltf::Model const& gltf_model,
			vren::render_list& render_list,
			vren::tinygltf_scene& result
		);

	public:
		tinygltf_loader(std::shared_ptr<vren::context> const& ctx);

		void load_from_file(
			std::filesystem::path const& model_filename,
			vren::render_list& render_list,
			vren::tinygltf_scene& result
		);
	};
}
