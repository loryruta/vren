#include "obj_loader.hpp"

#include <iostream>
#include <unordered_set>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

bool vren_demo::load_scene(std::filesystem::path const& in_file, vren_demo::scene& result)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warning;
	std::string error;
	bool ret = tinyobj::LoadObj(
		&attrib,
		&shapes,
		&materials,
		&warning,
		&error,
		in_file.string().c_str(),
		in_file.parent_path().string().c_str(),
		true, // triangulate
		false
	);

	if (!warning.empty()) {
		std::cout << warning << std::endl;
	}

	if (!error.empty()) {
		std::cerr << error << std::endl;
	}

	if (!ret) {
		return false;
	}

	for (tinyobj::shape_t& shape : shapes)
	{
		size_t idx_offset = 0;
		for (size_t face_vtx_num : shape.mesh.num_face_vertices)
		{
			if (face_vtx_num != 3) {
				std::cout << "Face isn't triangular. It has " << face_vtx_num << " vertices!" << std::endl;
			} else {
				for (size_t vtx_idx = 0; vtx_idx < face_vtx_num; vtx_idx++)
				{
					auto idx = shape.mesh.indices.at(idx_offset + vtx_idx);

					// Adds the index
					result.m_indices.emplace_back(result.m_vertices.size());

					// Adds the vertex
					vren::vertex& v = result.m_vertices.emplace_back(); // TODO (FIX-ME) one vertex per index (even for duplicated indices)

					v.m_position.x = attrib.vertices[3 * idx.vertex_index + 0];
					v.m_position.y = attrib.vertices[3 * idx.vertex_index + 1];
					v.m_position.z = attrib.vertices[3 * idx.vertex_index + 2];

					if (idx.normal_index >= 0) {
						v.m_normal.x = attrib.normals[3 * idx.normal_index + 0];
						v.m_normal.y = attrib.normals[3 * idx.normal_index + 1];
						v.m_normal.z = attrib.normals[3 * idx.normal_index + 2];
					}

					if (idx.texcoord_index >= 0) {
						v.m_texcoord.x = attrib.texcoords[2 * idx.texcoord_index + 0];
						v.m_texcoord.y = attrib.texcoords[2 * idx.texcoord_index + 1];
					}
				}

				// TODO load material
			}
			idx_offset += face_vtx_num;
		}
	}

	return true;
}

