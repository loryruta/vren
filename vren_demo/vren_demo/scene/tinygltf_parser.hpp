#pragma once

#include <memory>
#include <vector>

#include "tiny_gltf.h"

#include "vren/context.hpp"
#include "vren/vk_helpers/image.hpp"
#include "vren/texture_manager.hpp"
#include "vren/gpu_repr.hpp"

#include "intermediate_scene.hpp"

namespace vren_demo
{
	class tinygltf_parser
	{
	private:
		vren::context const* m_context;

	public:
		tinygltf_parser(vren::context const& context);

	private:
		void load_textures(std::filesystem::path const& model_folder, tinygltf::Model const& gltf_model);
		void load_materials(tinygltf::Model const& gltf_model, size_t texture_offset);

		void linearize_node_hierarchy(
			tinygltf::Model const& gltf_model,
			tinygltf::Node const& gltf_node,
			glm::mat4 transform,
			std::vector<std::vector<vren::mesh_instance>>& mesh_instances
		);

		void load_meshes(
			tinygltf::Model const& gltf_model,
			size_t material_offset,
			vren_demo::intermediate_scene& parsed_scene
		);

		void load_model(
			std::filesystem::path const& model_folder,
			tinygltf::Model const& gltf_model,
			vren_demo::intermediate_scene& parsed_scene
		);

	public:
		void load_from_file(
			std::filesystem::path const& model_filename,
			vren_demo::intermediate_scene& parsed_scene
		);
	};
}
