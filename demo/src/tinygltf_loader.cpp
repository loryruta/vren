#include "tinygltf_loader.hpp"

#include <stdexcept>
#include <optional>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/matrix_transform.hpp>

#include "vk_wrappers.hpp"


glm::vec3 parse_gltf_vec3_to_glm_vec3(std::vector<double> gltf_vec3)
{
	glm::vec3 r{};
	r.x = (float) gltf_vec3.at(0);
	r.y = (float) gltf_vec3.at(1);
	r.z = (float) gltf_vec3.at(2);
	return r;
}

glm::vec4 parse_gltf_vec4_to_glm_vec4(std::vector<double> gltf_vec4)
{
	glm::vec4 r{};
	r.x = (float) gltf_vec4.at(0);
	r.y = (float) gltf_vec4.at(1);
	r.z = (float) gltf_vec4.at(2);
	r.w = (float) gltf_vec4.at(3);
	return r;
}

glm::mat4 parse_gltf_mat4_to_glm_mat4(std::vector<double> m)
{
	return glm::mat4(
		m[0], m[4], m[8],  m[12],
		m[1], m[5], m[9],  m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]
	);
}

uint8_t const* get_accessor_element_at(tinygltf::Model const& model, tinygltf::Accessor const& accessor, size_t idx)
{
	tinygltf::BufferView const& buf_view = model.bufferViews.at(accessor.bufferView);
	tinygltf::Buffer const& buf = model.buffers.at(buf_view.buffer);

	size_t byte_off = buf_view.byteOffset + accessor.byteOffset;
	size_t stride = accessor.ByteStride(buf_view);
	size_t pos = byte_off + stride * idx;

	return buf.data.data() + pos;
}

VkFilter parse_filter(int gltf_filter)
{
	switch (gltf_filter)
	{
	case TINYGLTF_TEXTURE_FILTER_NEAREST: return VK_FILTER_NEAREST;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:  return VK_FILTER_LINEAR;
	default:                              return VK_FILTER_LINEAR; // todo
	}
}

VkSamplerAddressMode parse_address_mode(int gltf_address_mode)
{
	switch (gltf_address_mode)
	{
	case TINYGLTF_TEXTURE_WRAP_REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	default:                                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

vren::tinygltf_loader::tinygltf_loader(vren::renderer& renderer) :
	m_renderer(renderer)
{
}

void vren::tinygltf_loader::load_textures(
	std::filesystem::path const& model_dir,
	tinygltf::Model const& gltf_model,
	vren::tinygltf_scene& result
)
{
	result.m_textures.resize(gltf_model.textures.size());

	for (int i = 0; i < gltf_model.textures.size(); i++)
	{
		tinygltf::Texture const& gltf_tex = gltf_model.textures.at(i);

		tinygltf::Image const& gltf_img = gltf_model.images.at(gltf_tex.source);

		int img_w, img_h, img_comp;
		stbi_uc* img_data;

		if (gltf_img.bufferView >= 0) // loads the image in the gltf bundle itself
		{
			auto const& gltf_buf_view = gltf_model.bufferViews.at(gltf_img.bufferView);
			auto const& gltf_buf = gltf_model.buffers.at(gltf_buf_view.buffer);

			size_t byte_off = gltf_buf_view.byteOffset;
			size_t byte_str = gltf_buf_view.byteStride;
			size_t byte_len = gltf_buf_view.byteLength;

			if (byte_str != 0)
			{
				throw std::runtime_error("Unsupported image data format (byte stride > 0)");
			}

			img_data = stbi_load_from_memory(gltf_buf.data.data() + byte_off, (int) byte_len, &img_w, &img_h, &img_comp, STBI_rgb_alpha);
			if (img_data == nullptr)
			{
				fprintf(stderr, "Texture \"%s\" loading failed: %s\n", gltf_tex.name.c_str(), stbi_failure_reason());
				fflush(stderr);

				throw std::runtime_error("Unsupported image data");
			}
		}
		else // loads the image from an external file
		{
			std::filesystem::path img_file = model_dir / gltf_img.uri;

			std::cout << "Loading image at: " << img_file << std::endl;

			img_data = stbi_load(img_file.string().c_str(), &img_w, &img_h, &img_comp, STBI_rgb_alpha);
			if (img_data == nullptr)
			{
				std::cerr << "Texture \"" << img_file << "\"" << " loading failed: " << stbi_failure_reason();

				throw std::runtime_error("Unsupported image data");
			}
		}

		tinygltf::Sampler const& gltf_sampler = gltf_model.samplers.at(gltf_tex.sampler);

		auto tex = vren::make_rc<vren::texture>();
		vren::create_texture(
			m_renderer,
			img_w,
			img_h,
			img_data,
			VK_FORMAT_R8G8B8A8_UNORM,
			parse_filter(gltf_sampler.magFilter),
			parse_filter(gltf_sampler.minFilter),
			VK_SAMPLER_MIPMAP_MODE_NEAREST, // todo
			parse_address_mode(gltf_sampler.wrapR),
			parse_address_mode(gltf_sampler.wrapS),
			parse_address_mode(gltf_sampler.wrapT),
			*tex
		);
		result.m_textures[i] = tex;

		stbi_image_free(img_data);
	}
}

void vren::tinygltf_loader::load_materials(
	tinygltf::Model const& gltf_model,
	vren::tinygltf_scene& result
)
{
	result.m_materials.resize(gltf_model.materials.size());

	for (int i = 0; i < gltf_model.materials.size(); i++)
	{
		tinygltf::Material const& gltf_mat = gltf_model.materials.at(i);
		tinygltf::PbrMetallicRoughness const& gltf_pbr = gltf_mat.pbrMetallicRoughness;

		auto mat = vren::make_rc<vren::material>(m_renderer);

		mat->m_base_color_factor = parse_gltf_vec4_to_glm_vec4(gltf_pbr.baseColorFactor);
		mat->m_metallic_factor   = (float) gltf_pbr.metallicFactor;
		mat->m_roughness_factor  = (float) gltf_pbr.roughnessFactor;

		mat->m_base_color_texture = result.m_textures.at(gltf_pbr.baseColorTexture.index);
		mat->m_metallic_roughness_texture = result.m_textures.at(gltf_pbr.metallicRoughnessTexture.index);

		result.m_materials[i] = mat;
	}
}

void vren::tinygltf_loader::linearize_node_hierarchy(
	tinygltf::Model const& gltf_model,
	tinygltf::Node const& gltf_node,
	glm::mat4 transform,
	std::vector<std::vector<vren::instance_data>>& instance_data_by_mesh_idx
)
{
	if (!gltf_node.matrix.empty()) // if matrix is present
	{
		transform = transform * parse_gltf_mat4_to_glm_mat4(gltf_node.matrix);
	}

	if (gltf_node.translation.size() == 3)
	{
		transform = glm::translate(transform, parse_gltf_vec3_to_glm_vec3(gltf_node.translation));
	}

	// todo quaternion rotation

	if (gltf_node.scale.size() == 3)
	{
		transform = glm::scale(transform, parse_gltf_vec3_to_glm_vec3(gltf_node.scale));
	}

	// Registers the node for the mesh
	if (gltf_node.mesh >= 0)
	{
		vren::instance_data inst_data{};
		inst_data.m_transform = transform;

		instance_data_by_mesh_idx[gltf_node.mesh].push_back(inst_data);
	}

	// Traverses children
	if (gltf_node.children.size() > 0)
	{
		for (int child_idx : gltf_node.children)
		{
			linearize_node_hierarchy(gltf_model, gltf_model.nodes.at(child_idx), transform, instance_data_by_mesh_idx);
		}
	}
}

void vren::tinygltf_loader::load_model(
	std::filesystem::path const& model_dir,
	tinygltf::Model const& gltf_model,
	vren::render_list& render_list,
	vren::tinygltf_scene& result
)
{
	// Textures
	load_textures(model_dir, gltf_model, result);

	// Materials
	load_materials(gltf_model, result);

	// Instances
	std::vector<std::vector<vren::instance_data>> instance_data_by_mesh_idx;
	instance_data_by_mesh_idx.resize(gltf_model.meshes.size());

	for (tinygltf::Scene const& gltf_scene : gltf_model.scenes) // we load all the scenes
	{
		for (int node : gltf_scene.nodes)
		{
			linearize_node_hierarchy(gltf_model, gltf_model.nodes.at(node), glm::identity<glm::mat4>(), instance_data_by_mesh_idx);
		}
	}

	// Meshes
	for (int mesh_idx = 0; mesh_idx < gltf_model.meshes.size(); mesh_idx++)
	{
		tinygltf::Mesh const& gltf_mesh = gltf_model.meshes.at(mesh_idx);

		if (gltf_mesh.primitives.size() != 1)
		{
			throw std::runtime_error("A mesh must have exactly one primitive");
		}

		tinygltf::Primitive const& gltf_primitive = gltf_mesh.primitives.at(0);

		if (gltf_primitive.mode != TINYGLTF_MODE_TRIANGLES)
		{
			throw std::runtime_error("Primitive mode != TRIANGLES not supported");
		}

		auto& render_obj = render_list.create_render_object();

		if (!gltf_primitive.attributes.count("POSITION"))
		{
			throw std::runtime_error("Mesh POSITION attribute must be present");
		}

		using opt_accessor_t = std::optional<std::reference_wrapper<tinygltf::Accessor const>>;

		opt_accessor_t
			pos_accessor,
			norm_accessor,
			tan_accessor,
			texcoord_0_accessor;

		{ // POSITION attribute accessor
			pos_accessor = gltf_model.accessors.at(gltf_primitive.attributes.at("POSITION"));
		}

		// NORMAL attribute accessor
		if (gltf_primitive.attributes.count("NORMAL"))
		{
			norm_accessor = gltf_model.accessors.at(gltf_primitive.attributes.at("NORMAL"));
		}

		// TANGENT attribute accessor
		if (gltf_primitive.attributes.count("TANGENT"))
		{
			tan_accessor = gltf_model.accessors.at(gltf_primitive.attributes.at("TANGENT"));
		}

		// TEXCOORD_0 attribute accessor
		if (gltf_primitive.attributes.count("TEXCOORD_0"))
		{
			texcoord_0_accessor = gltf_model.accessors.at(gltf_primitive.attributes.at("TEXCOORD_0"));
		}

		size_t vtx_count = pos_accessor->get().count;

		if (
			(norm_accessor.has_value()       && norm_accessor->get().count       != vtx_count) ||
			(tan_accessor.has_value()        && tan_accessor->get().count        != vtx_count) ||
			(texcoord_0_accessor.has_value() && texcoord_0_accessor->get().count != vtx_count)
		)
		{
			throw std::runtime_error("POSITION vertices count mismatches with at least one of other attributes count");
		}

		{ // vertices
			std::vector<vren::vertex> vertices(vtx_count);

			for (int i = 0; i < vtx_count; i++)
			{
				auto& v = vertices[i];

				// todo verify accessor type and component type match before reading them!

				if (pos_accessor.has_value()) // position
				{
					v.m_position = *reinterpret_cast<glm::vec3 const*>(
						get_accessor_element_at(gltf_model, pos_accessor.value(), i)
					);
				}

				if (norm_accessor.has_value()) // normal
				{
					v.m_normal = *reinterpret_cast<glm::vec3 const*>(
						get_accessor_element_at(gltf_model, norm_accessor.value(), i)
					);
				}

				if (tan_accessor.has_value()) // tangent
				{
					v.m_tangent = *reinterpret_cast<glm::vec3 const*>(
						get_accessor_element_at(gltf_model, tan_accessor.value(), i)
					);
				}

				if (texcoord_0_accessor.has_value()) // texcoord 0
				{
					v.m_texcoord = *reinterpret_cast<glm::vec2 const*>(
						get_accessor_element_at(gltf_model, texcoord_0_accessor.value(), i)
					);
				}
			}

			render_obj.set_vertices_data(vertices.data(), vertices.size());
		}

		{ // indices
			tinygltf::Accessor const& indices_accessor = gltf_model.accessors.at(gltf_primitive.indices);

			std::vector<uint32_t> indices(indices_accessor.count);

			for (int i = 0; i < indices_accessor.count; i++)
			{
				uint32_t idx;

				switch (indices_accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_BYTE:
					idx = *reinterpret_cast<uint8_t const*>(
						get_accessor_element_at(gltf_model, indices_accessor, i)
					);
					break;
				case TINYGLTF_COMPONENT_TYPE_SHORT:
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					idx = *reinterpret_cast<uint16_t const*>(
						get_accessor_element_at(gltf_model, indices_accessor, i)
					);
					break;
				case TINYGLTF_COMPONENT_TYPE_INT:
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					idx = *reinterpret_cast<uint32_t const*>(
						get_accessor_element_at(gltf_model, indices_accessor, i)
					);
					break;
				default:
					// should never reach this point
					throw std::invalid_argument("Unsupported indices component type");
				}

				indices[i] = idx;
			}

			render_obj.set_indices_data(indices.data(), indices.size());
		}

		// instances
		auto& instances_data = instance_data_by_mesh_idx.at(mesh_idx);
		render_obj.set_instances_data(instances_data.data(), instances_data.size());

		// material
		render_obj.m_material = result.m_materials.at(gltf_primitive.material);

		result.m_render_objects.push_back(render_obj.m_idx); // register the id of the render object for the loaded scene
	}
}

void vren::tinygltf_loader::load_from_file(
	std::filesystem::path const& model_file,
	vren::render_list& render_list,
	vren::tinygltf_scene& result
)
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf_model;
	std::string warn, err;

	bool ret = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, model_file.string());

	if (!warn.empty())
	{
		printf("Loading warning: %s\n", warn.c_str());
		fflush(stderr);
	}

	if (!err.empty())
	{
		printf("Scene loading failed: %s\n", err.c_str());
		fflush(stderr);
	}

	if (!ret)
	{
		throw std::runtime_error("Scene loading failed");
	}

	load_model(model_file.parent_path(), gltf_model, render_list, result);
}
