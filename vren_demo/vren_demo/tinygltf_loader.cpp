#include "tinygltf_loader.hpp"

#include <stdexcept>
#include <optional>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/matrix_transform.hpp>

#include <vren/utils/profiler.hpp>

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

VkFilter parse_gltf_filter(int gltf_filter)
{
	switch (gltf_filter)
	{
	case TINYGLTF_TEXTURE_FILTER_NEAREST: return VK_FILTER_NEAREST;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:  return VK_FILTER_LINEAR;
	default:                              return VK_FILTER_LINEAR; // todo
	}
}

VkSamplerAddressMode parse_gltf_address_mode(int gltf_address_mode)
{
	switch (gltf_address_mode)
	{
	case TINYGLTF_TEXTURE_WRAP_REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	default:                                    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}
}

vren_demo::tinygltf_loader::tinygltf_loader(vren::context const& ctx) :
	m_context(&ctx)
{}

void vren_demo::tinygltf_loader::load_textures(
	std::filesystem::path const& model_dir,
	tinygltf::Model const& gltf_model,
	vren::texture_manager& tex_manager
)
{
	for (auto const& gltf_tex : gltf_model.textures)
	{
		tinygltf::Image const& gltf_img = gltf_model.images.at(gltf_tex.source);

		int img_w, img_h, img_comp;
		stbi_uc* img_data;

		if (gltf_img.bufferView >= 0)
		{ /* Bundle texture loading */
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
				fprintf(stderr, "Failed to load texture %s, reason: %s\n", gltf_tex.name.c_str(), stbi_failure_reason());
				throw std::runtime_error("Unsupported image data");
			}
		}
		else
		{ /* From file texture loading */
			std::filesystem::path img_file = model_dir / gltf_img.uri;

			//printf("Loading texture: %ls\n", img_file.c_str());

			img_data = stbi_load(img_file.string().c_str(), &img_w, &img_h, &img_comp, STBI_rgb_alpha);

			if (img_data == nullptr)
			{
				fprintf(stderr, "Failed to load texture %ls, reason: %s\n", img_file.c_str(), stbi_failure_reason());
				throw std::runtime_error("Unsupported image data");
			}
		}

		/* Sampler */
		VkFilter
			min_filter = VK_FILTER_LINEAR,
			mag_filter = VK_FILTER_LINEAR;
		VkSamplerAddressMode
			address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		if (gltf_tex.sampler >= 0)
		{
			auto& gltf_sampler = gltf_model.samplers.at(gltf_tex.sampler);

			min_filter = parse_gltf_filter(gltf_sampler.minFilter);
			mag_filter = parse_gltf_filter(gltf_sampler.magFilter);
			address_mode_u = parse_gltf_address_mode(gltf_sampler.wrapR);
			address_mode_v = parse_gltf_address_mode(gltf_sampler.wrapS);
			address_mode_w = parse_gltf_address_mode(gltf_sampler.wrapT);
		}

		/* */

		tex_manager.m_textures.push_back(
			vren::vk_utils::create_texture(*m_context, img_w, img_h, img_data, VK_FORMAT_R8G8B8A8_UNORM, mag_filter, min_filter, VK_SAMPLER_MIPMAP_MODE_NEAREST, address_mode_u, address_mode_v, address_mode_w)
		);

		stbi_image_free(img_data);
	}
}

void vren_demo::tinygltf_loader::load_materials(
	tinygltf::Model const& gltf_model,
	std::vector<material>& materials,
	size_t tex_off
)
{
	for (auto const& gltf_mat : gltf_model.materials)
	{
		tinygltf::PbrMetallicRoughness const& gltf_pbr = gltf_mat.pbrMetallicRoughness;
		materials.push_back({
			.m_base_color_texture_idx = gltf_pbr.baseColorTexture.index >= 0 ? (uint32_t) (tex_off + gltf_pbr.baseColorTexture.index) : 0,
			.m_metallic_roughness_texture_idx = gltf_pbr.metallicRoughnessTexture.index >= 0 ? (uint32_t) (tex_off + gltf_pbr.metallicRoughnessTexture.index) : 0,
			.m_base_color_factor = parse_gltf_vec4_to_glm_vec4(gltf_pbr.baseColorFactor),
			.m_metallic_factor = (float) gltf_pbr.metallicFactor,
			.m_roughness_factor = (float) gltf_pbr.roughnessFactor
		});
	}
}

void vren_demo::tinygltf_loader::linearize_node_hierarchy(
	tinygltf::Model const& gltf_model,
	tinygltf::Node const& gltf_node,
	glm::mat4 transform,
	std::vector<std::vector<mesh_instance>>& mesh_instances
)
{
	if (!gltf_node.matrix.empty()) { // If matrix is present
		transform = transform * parse_gltf_mat4_to_glm_mat4(gltf_node.matrix);
	}

	if (gltf_node.translation.size() == 3) {
		transform = glm::translate(transform, parse_gltf_vec3_to_glm_vec3(gltf_node.translation));
	}

	// TODO quaternion rotation

	if (gltf_node.scale.size() == 3) {
		transform = glm::scale(transform, parse_gltf_vec3_to_glm_vec3(gltf_node.scale));
	}

	// Registers the node for the mesh
	if (gltf_node.mesh >= 0) {
		mesh_instances[gltf_node.mesh].push_back({
			.m_transform = transform
		});
	}

	// Traverses children
	if (gltf_node.children.size() > 0) {
		for (int child_idx : gltf_node.children) {
			linearize_node_hierarchy(gltf_model, gltf_model.nodes.at(child_idx), transform, mesh_instances);
		}
	}
}

void vren_demo::tinygltf_loader::load_model(
	std::filesystem::path const& model_dir,
	tinygltf::Model const& gltf_model,
	std::vector<vertex>& vertices,
	std::vector<uint32_t>& indices,
	std::vector<mesh_instance>& mesh_instances,
	vren::texture_manager& tex_manager,
	std::vector<material>& materials,
	std::vector<mesh>& meshes
)
{
	double elapsed_ms;

	/* Textures */
	size_t tex_off = tex_manager.m_textures.size();
	elapsed_ms = vren::profile_ms([&](){ load_textures(model_dir, gltf_model, tex_manager); });
	printf("Textures loaded in: %.2f ms\n", elapsed_ms);

	/* Materials */
	size_t mat_off = materials.size();
	elapsed_ms = vren::profile_ms([&](){ load_materials(gltf_model, materials, tex_off); });
	printf("Materials loaded in: %.2f ms\n", elapsed_ms);

	/* Meshes */
	for (size_t mesh_idx = 0; mesh_idx < gltf_model.meshes.size(); mesh_idx++)
	{
		auto const& gltf_mesh = gltf_model.meshes.at(mesh_idx);

		mesh mesh;

		for (auto const& gltf_primitive : gltf_mesh.primitives)
		{
			//assert(gltf_primitive.mode == TINYGLTF_MODE_TRIANGLES);
			if (gltf_primitive.mode != TINYGLTF_MODE_TRIANGLES) {
				continue;
			}

			auto const& attribs = gltf_primitive.attributes;

			assert(attribs.contains("POSITION"));
			assert(attribs.contains("NORMAL"));

			auto const& position_accessor = gltf_model.accessors.at(attribs.at("POSITION"));
			auto const& normal_accessor = gltf_model.accessors.at(attribs.at("NORMAL"));
			auto const& texcoord_accessor = attribs.contains("TEXCOORD_0") ? std::make_optional(gltf_model.accessors.at(attribs.at("TEXCOORD_0"))) : std::nullopt;
			// TANGENT

			assert(position_accessor.type == TINYGLTF_TYPE_VEC3 && position_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
			assert(normal_accessor.type == TINYGLTF_TYPE_VEC3 && normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
			assert(!texcoord_accessor.has_value() || (texcoord_accessor->type == TINYGLTF_TYPE_VEC2 && texcoord_accessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT));

			/* Vertices */
			size_t vtx_count = position_accessor.count;
			assert(vtx_count == normal_accessor.count);
			assert(!texcoord_accessor.has_value() || vtx_count == texcoord_accessor->count);

			mesh.m_vertex_count = vtx_count;
			mesh.m_vertex_offset = vertices.size();

			for (size_t vtx_idx = 0; vtx_idx < vtx_count; vtx_idx++)
			{
				vertex vtx;
				vtx.m_position = *reinterpret_cast<glm::vec3 const*>(get_accessor_element_at(gltf_model, position_accessor, vtx_idx));
				vtx.m_normal = *reinterpret_cast<glm::vec3 const*>(get_accessor_element_at(gltf_model, normal_accessor, vtx_idx));
				if (texcoord_accessor.has_value()) {
					vtx.m_texcoord = *reinterpret_cast<glm::vec2 const*>(get_accessor_element_at(gltf_model, texcoord_accessor.value(), vtx_idx));
				}
				vertices.push_back(vtx);
			}

			/* Indices */
			auto const& indices_accessor = gltf_model.accessors.at(gltf_primitive.indices);

			mesh.m_index_count = indices_accessor.count;
			mesh.m_index_offset = indices.size();

			for (size_t i = 0; i < indices_accessor.count; i++)
			{
				uint32_t local_index;
				switch (indices_accessor.componentType)
				{
				case TINYGLTF_COMPONENT_TYPE_BYTE:
					local_index = *reinterpret_cast<uint8_t const*>(
						get_accessor_element_at(gltf_model, indices_accessor, i)
					);
					break;
				case TINYGLTF_COMPONENT_TYPE_SHORT:
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					local_index = *reinterpret_cast<uint16_t const*>(
						get_accessor_element_at(gltf_model, indices_accessor, i)
					);
					break;
				case TINYGLTF_COMPONENT_TYPE_INT:
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					local_index = *reinterpret_cast<uint32_t const*>(
						get_accessor_element_at(gltf_model, indices_accessor, i)
					);
					break;
				default:
					throw std::invalid_argument("Unsupported indices component type");
				}
				indices.push_back(local_index);
			}

			/* Material */
			mesh.m_material_idx = mat_off + gltf_primitive.material;

			/*  */

			meshes.push_back(mesh);
		}
	}

	/* Instances */
	std::vector<std::vector<mesh_instance>> instances_by_mesh(gltf_model.meshes.size());
	for (auto const& gltf_scene : gltf_model.scenes)
	{
		for (int node : gltf_scene.nodes) {
			linearize_node_hierarchy(gltf_model, gltf_model.nodes.at(node), glm::identity<glm::mat4>(), instances_by_mesh);
		}
	}

	for (size_t mesh_idx = 0; mesh_idx < instances_by_mesh.size(); mesh_idx++)
	{
		auto& mesh = meshes.at(mesh_idx);
		auto const& per_mesh_instances = instances_by_mesh.at(mesh_idx);

		mesh.m_instance_count = per_mesh_instances.size();
		mesh.m_instance_offset = mesh_instances.size();

		mesh_instances.insert(mesh_instances.end(), per_mesh_instances.begin(), per_mesh_instances.end());
	}
}

void vren_demo::tinygltf_loader::load_from_file(
	std::filesystem::path const& model_filename,
	std::vector<vertex>& vertices,
	std::vector<uint32_t>& indices,
	std::vector<mesh_instance>& instances,
	vren::texture_manager& texture_manager,
	std::vector<material>& materials,
	std::vector<mesh>& meshes
)
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf_model;
	std::string warn, err;

	printf("Loading model file: %ls\n", model_filename.c_str());

	bool ret;
	if (model_filename.extension() == ".gltf")     ret = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, model_filename.string());
	else if (model_filename.extension() == ".glb") ret = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, model_filename.string());
	else
	{
		throw std::runtime_error("Unsupported file extension");
	}

	if (!warn.empty())
	{
		fprintf(stderr, "%s loading warning: %s\n", model_filename.string().c_str(), warn.c_str());
		throw std::runtime_error("Loading failed");
	}

	if (!err.empty())
	{
		fprintf(stderr, "%s loading error: %s\n", model_filename.string().c_str(), err.c_str());
		throw std::runtime_error("Loading failed");
	}

	if (!ret)
	{
		throw std::runtime_error("Scene loading failed");
	}

	size_t vertex_off = vertices.size();
	size_t index_off = indices.size();
	size_t instance_off = instances.size();
	size_t tex_off = texture_manager.m_textures.size();
	size_t mat_off = materials.size();
	size_t meshes_off = meshes.size();

	double elapsed_ms =
		vren::profile_ms([&]() {
			load_model(
				model_filename.parent_path(),
				gltf_model,
				vertices, indices, instances, texture_manager, materials, meshes
			);
		});

	printf("Loaded %lld vertices, %lld indices, %lld instances, %lld textures, %lld materials, %lld meshes in %.2f ms\n",
		vertices.size() - vertex_off,
		indices.size() - index_off,
		instances.size() - instance_off,
		texture_manager.m_textures.size() - tex_off,
		materials.size() - mat_off,
		meshes.size() - meshes_off,
		   elapsed_ms
   );
}
