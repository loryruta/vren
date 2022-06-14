#include "basic_model_uploader.hpp"

#include "gpu_repr.hpp"
#include "log.hpp"

vren::basic_model_draw_buffer vren::basic_model_uploader::upload(vren::context const& context, vren::model const& model)
{
	std::vector<vren::basic_model_draw_buffer::mesh> meshes;
	meshes.reserve(model.m_meshes.size());
	for (vren::model::mesh const& mesh : model.m_meshes)
	{
		meshes.push_back(vren::basic_model_draw_buffer::mesh{
			.m_vertex_offset   = mesh.m_vertex_offset,
			.m_vertex_count    = mesh.m_vertex_count,
			.m_index_offset    = mesh.m_index_offset,
			.m_index_count     = mesh.m_index_count,
			.m_instance_offset = mesh.m_instance_offset,
			.m_instance_count  = mesh.m_instance_count,
			.m_material_idx    = mesh.m_material_idx
		});
	}

	vren::basic_model_draw_buffer draw_buffer{
		.m_name            = model.m_name,
		.m_vertex_buffer   = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, model.m_vertices.data(), model.m_vertices.size() * sizeof(vren::vertex)),
		.m_index_buffer    = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, model.m_indices.data(), model.m_indices.size() * sizeof(uint32_t)),
		.m_instance_buffer = vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, model.m_instances.data(), model.m_instances.size() * sizeof(vren::mesh_instance)),
		.m_meshes          = std::move(meshes)
	};

	draw_buffer.set_object_names(context);

	VREN_INFO("[basic_model_uploader] Uploaded model: {}\n", draw_buffer.m_name);

	return draw_buffer;
}
