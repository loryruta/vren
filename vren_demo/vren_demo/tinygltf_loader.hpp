#pragma once

#include <memory>
#include <vector>

#include <tiny_gltf.h>

#include <vren/context.hpp>
#include <vren/vk_helpers/image.hpp>
#include <vren/texture_manager.hpp>
#include <vren/mesh.hpp>

namespace vren_demo
{
	struct tinygltf_scene
	{
		std::vector<float> m_vertices;
		std::vector<uint32_t> m_indices;

		size_t m_triangles_count = 0;
	};

	class tinygltf_loader
	{
	private:
		vren::context const* m_context;

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
			std::vector<vren::vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<vren::mesh>& meshes,
			std::vector<vren::mesh_instance>& mesh_instances,
			size_t material_offset
		);

		void load_mesh_instances(
			tinygltf::Model const& gltf_model,
			std::vector<vren::mesh>& meshes,
			std::vector<vren::mesh_instance>& mesh_instances
		);

		void load_model(
			std::filesystem::path const& model_folder,
			tinygltf::Model const& gltf_model,
			std::vector<vren::vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<vren::mesh>& meshes,
			std::vector<vren::mesh_instance>& mesh_instances
		);

	public:
		tinygltf_loader(vren::context const& ctx);

		void load_from_file(
			std::filesystem::path const& model_filename,
			std::vector<vren::vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<vren::mesh>& meshes,
			std::vector<vren::mesh_instance>& mesh_instances
		);
	};
}
