#include "ai_scene_loader.hpp"

#include <iostream>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "material.hpp"

glm::vec3 to_glm_vec3(aiVector3D const& v)
{
	return glm::vec3(v.x, v.y, v.z);
}

glm::vec4 to_glm_vec4(aiColor4D const& v)
{
	return glm::vec4(v.r, v.g, v.b, v.a);
}

glm::mat4 to_glm_mat4(aiMatrix4x4 const& v)
{
	glm::mat4 r;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			r[i][j] = v[i][j];
		}
	}
	return r;
}

void load_mesh_instances(
	aiScene const* ai_scene,
	aiNode const* ai_node,
	aiMatrix4x4 transf,
	std::unordered_map<uint32_t, std::vector<vren::instance_data>>& per_mesh_instances)
{
	transf *= ai_node->mTransformation;

	for (int i = 0; i < ai_node->mNumMeshes; i++)
	{
		auto [it, placed] = per_mesh_instances.emplace(ai_node->mMeshes[i], std::vector<vren::instance_data>{});

		vren::instance_data data{};
		data.m_transform = to_glm_mat4(transf);
		it->second.emplace_back(data);
	}

	for (int i = 0; i < ai_node->mNumChildren; i++)
	{
		load_mesh_instances(ai_scene, ai_node->mChildren[i], transf, per_mesh_instances);
	}
}

vren_demo::ai_scene_baker::ai_scene_baker(vren::renderer& renderer) :
	m_renderer(renderer)
{
}

vren_demo::ai_scene_baker::~ai_scene_baker()
{
}

void vren_demo::ai_scene_baker::create_texture(aiTexture const* ai_texture, vren::texture& result)
{
	int w, h;
	int ch;

	size_t len = ai_texture->mWidth * (ai_texture->mHeight == 0 ? 1 : ai_texture->mHeight);
	uint8_t* pixels = stbi_load_from_memory(reinterpret_cast<uint8_t*>(ai_texture->pcData), len, &w, &h, &ch, STBI_rgb_alpha);

	if(!pixels)
	{
		throw std::runtime_error("STBI couldn't load in-memory image");
	}

	vren::create_texture(
		m_renderer,
		w,
		h,
		pixels,
		VK_FORMAT_R8G8B8A8_UNORM,
		result
	);

	stbi_image_free(pixels);
}

void vren_demo::ai_scene_baker::create_material_texture(
	aiScene const* ai_scene,
	aiMaterial const* ai_mat,
	char const* texture_key, unsigned int texture_type, unsigned int texture_index,
	vren::texture& result
)
{
	aiString filename;

	aiGetMaterialString(ai_mat, texture_key, texture_type, texture_index, &filename);
	aiTexture const* ai_tex = ai_scene->GetEmbeddedTexture(filename.C_Str());
	if (ai_tex)
	{
		create_texture(ai_tex, result);
	}
}

void vren_demo::ai_scene_baker::bake(aiScene const* ai_scene, vren::render_list& render_list)
{
	std::vector<vren::material*> materials{};

	materials.resize(ai_scene->mNumMaterials);

	for (int i = 0; i < ai_scene->mNumMaterials; i++)
	{
		auto ai_mat = ai_scene->mMaterials[i];

		vren::material* material = m_renderer.m_material_manager->create_material();

		create_material_texture(ai_scene, ai_mat, AI_MATKEY_TEXTURE_DIFFUSE(0),  material->m_albedo_texture);

		material->m_metallic  = 1.0f;
		material->m_roughness = 0.5f;

		materials[i] = material;
	}

	for (int i = 0; i < ai_scene->mNumMeshes; i++)
	{
		auto ai_mesh = ai_scene->mMeshes[i];

		//printf("Creating mesh: %s\n", ai_mesh->mName.C_Str());

		auto& render_obj = render_list.create_render_object();

		// Vertices
		std::vector<vren::vertex> vertices;
		vertices.resize(ai_mesh->mNumVertices);

		for (int j = 0; j < ai_mesh->mNumVertices; j++)
		{
			vren::vertex v{};

			// Position
			v.m_position = to_glm_vec3(ai_mesh->mVertices[j]);

			// Normal
			if (ai_mesh->mNormals)
			{
				v.m_normal = to_glm_vec3(ai_mesh->mNormals[j]);
			}

			// Tangent
			if (ai_mesh->mTangents)
			{
				v.m_tangent = to_glm_vec3(ai_mesh->mTangents[j]);
			}

			// Tex coords
			if (ai_mesh->mTextureCoords[0])
			{
				v.m_texcoord = to_glm_vec3(ai_mesh->mTextureCoords[0][j]);
			}

			// Color
			if (ai_mesh->mColors[0])
			{
				v.m_color = to_glm_vec4(ai_mesh->mColors[0][j]);
			}

			vertices[j] = v;
		}

		render_obj.set_vertices_data(vertices.data(), vertices.size());

		// Indices
		std::vector<uint32_t> indices;
		indices.resize(ai_mesh->mNumFaces * 3);

		for (int j = 0; j < ai_mesh->mNumFaces; j++)
		{
			auto& ai_face = ai_mesh->mFaces[j];
			if (ai_face.mNumIndices != 3)
			{
				throw std::invalid_argument("Non-triangular faces aren't supported");
			}

			for (int k = 0; k < ai_face.mNumIndices; k++)
			{
				indices.push_back(ai_face.mIndices[k]);
			}
		}

		render_obj.set_indices_data(indices.data(), indices.size());

		// Material
		auto material_idx = ai_mesh->mMaterialIndex;
		render_obj.m_material = materials[material_idx];
	}

	// Instances
	std::unordered_map<uint32_t, std::vector<vren::instance_data>> per_mesh_instances{};
	load_mesh_instances(ai_scene, ai_scene->mRootNode, aiMatrix4x4(), per_mesh_instances);

	for (auto& [mesh_idx, instances_data] : per_mesh_instances)
	{
		auto& render_obj = render_list.get_render_object(mesh_idx);
		render_obj.set_instances_data(instances_data.data(), instances_data.size());
	}
}
