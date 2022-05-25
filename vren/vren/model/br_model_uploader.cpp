#include "br_model_uploader.hpp"

#include "log.hpp"
#include "gpu_repr.hpp"

vren::br::draw_buffer vren::br::model_uploader::upload(
	vren::context const& context,
	pre_upload_model_t const& model
)
{
	std::vector<draw_buffer::mesh> meshes;
	meshes.reserve(model.m_meshes.size());
	for (auto mesh : model.m_meshes)
	{
		meshes.push_back(draw_buffer::mesh{
			.m_vertex_offset   = mesh.m_vertex_offset,
			.m_vertex_count    = mesh.m_vertex_count,
			.m_index_offset    = mesh.m_index_offset,
			.m_index_count     = mesh.m_index_count,
			.m_instance_offset = mesh.m_instance_offset,
			.m_instance_count  = mesh.m_instance_count,
			.m_material_idx    = mesh.m_material_idx
		});
	}

	vren::br::draw_buffer draw_buffer{
		.m_name            = model.m_name,
		.m_vertex_buffer   = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, model.m_vertices.data(), model.m_vertices.size() * sizeof(vren::vertex)),
		.m_index_buffer    = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, model.m_indices.data(), model.m_indices.size() * sizeof(uint32_t)),
		.m_instance_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, model.m_instances.data(), model.m_instances.size() * sizeof(vren::mesh_instance)),
		.m_meshes          = std::move(meshes)
	};

	VREN_INFO("[br] [model_uploader] Uploaded model: {}\n", draw_buffer.m_name);

	return draw_buffer;
}
