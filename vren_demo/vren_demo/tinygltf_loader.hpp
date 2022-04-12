#pragma once

#include <memory>
#include <vector>

#include <tiny_gltf.h>

#include <vren/context.hpp>
#include <vren/render_list.hpp>
#include <vren/render_object.hpp>
#include <vren/material.hpp>
#include <vren/vk_helpers/image.hpp>
#include <vren/texture.hpp>

namespace vren_demo
{
	struct material
	{
		uint32_t m_base_color_texture_idx;
		uint32_t m_metallic_roughness_texture_idx;
		glm::vec4 m_base_color_factor;
		float m_metallic_factor;
		float m_roughness_factor;
	};

	struct vertex
	{
		glm::vec3 m_position;
		glm::vec3 m_normal;
		glm::vec2 m_texcoord;
	};

	struct mesh_instance
	{
		glm::mat4 m_transform;
	};

	struct mesh
	{
		size_t m_vertex_offset, m_vertex_count;
		size_t m_index_offset, m_index_count;
		size_t m_instance_offset, m_instance_count;

		size_t m_material_idx;
	};

	struct tinygltf_scene
	{
		std::vector<std::shared_ptr<vren::vk_utils::texture>> m_textures; // TODO no textures
		std::vector<std::shared_ptr<vren::material>> m_materials; // TODO no materials
		std::vector<uint32_t> m_render_objects; // TODO no render objects

		std::vector<float> m_vertices;
		std::vector<uint32_t> m_indices;

		size_t m_triangles_count = 0;
	};

	class tinygltf_loader
	{
	private:
		vren::context const* m_context;

		void load_textures(
			std::filesystem::path const& model_dir,
			tinygltf::Model const& gltf_model,
			vren::texture_manager& texture_manager
		);

		void load_materials(
			tinygltf::Model const& gltf_model,
			std::vector<material>& materials,
			size_t tex_off = 0
		);

		void linearize_node_hierarchy(
			tinygltf::Model const& gltf_model,
			tinygltf::Node const& gltf_node,
			glm::mat4 transform,
			std::vector<std::vector<mesh_instance>>& mesh_instances
		);

		void load_model(
			std::filesystem::path const& model_dir,
			tinygltf::Model const& gltf_model,
			std::vector<vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<mesh_instance>& instances,
			vren::texture_manager& texture_manager,
			std::vector<material>& materials,
			std::vector<mesh>& meshes
		);

	public:
		tinygltf_loader(vren::context const& ctx);

		void load_from_file(
			std::filesystem::path const& model_filename,
			std::vector<vertex>& vertices,
			std::vector<uint32_t>& indices,
			std::vector<mesh_instance>& mesh_instances,
			vren::texture_manager& texture_manager,
			std::vector<material>& materials,
			std::vector<mesh>& meshes
		);
	};
}
