#include "msr_model_uploader.hpp"

vren::msr::draw_buffer vren::msr::model_uploader::upload(vren::context const& context, pre_upload_model const& model)
{
	auto vertex_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, model.m_vertices.data(), model.m_vertices.size() * sizeof(vren::vertex));

	auto meshlet_vertex_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, model.m_meshlet_vertices.data(), model.m_meshlet_vertices.size() * sizeof(uint32_t));

	auto meshlet_triangle_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,  model.m_meshlet_vertices.data(), model.m_meshlet_vertices.size() * 3 * sizeof(uint8_t));

	auto meshlet_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, model.m_meshlets.data(), model.m_meshlets.size() * sizeof(vren::meshlet));

	auto instanced_meshlet_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, model.m_instanced_meshlets.data(), model.m_instanced_meshlets.size() * sizeof(vren::instanced_meshlet));

	auto instance_buffer =
		vren::vk_utils::create_device_only_buffer(context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, model.m_instances.data(), model.m_instances.size() * sizeof(vren::mesh_instance));

	vren::msr::draw_buffer draw_buffer{
		.m_name                     = model.m_name,
		.m_vertex_buffer            = std::move(vertex_buffer),
		.m_meshlet_vertex_buffer    = std::move(meshlet_vertex_buffer),
		.m_meshlet_triangle_buffer  = std::move(meshlet_triangle_buffer),
		.m_meshlet_buffer           = std::move(meshlet_buffer),
		.m_instanced_meshlet_buffer = std::move(instanced_meshlet_buffer),
		.m_instanced_meshlet_count  = model.m_instanced_meshlets.size(),
		.m_instance_buffer          = std::move(instance_buffer)
	};

	//vren::set_object_names(context, draw_buffer);

	VREN_INFO("[msr] [model_uploader] Uploading model: {}\n", draw_buffer.m_name);

	return std::move(draw_buffer);
}
