#pragma once

#include <string>

#include "model.hpp"
#include "msr_model_uploader.hpp"

namespace vren::msr
{
	struct pre_upload_model
	{
		std::string m_name = "unnamed";

		std::vector<vren::vertex> m_vertices;
		std::vector<uint32_t> m_meshlet_vertices;
		std::vector<uint8_t> m_meshlet_triangles;
		std::vector<vren::meshlet> m_meshlets;
		std::vector<vren::instanced_meshlet> m_instanced_meshlets;
		std::vector<vren::mesh_instance> m_instances;
	};

	class model_specializer
	{
	private:
		void reserve_buffer_space(pre_upload_model& output, vren::model const& model);
		void clusterize_mesh(pre_upload_model& output, vren::model const& model, vren::model::mesh const& mesh);
		void clusterize_model(pre_upload_model& output, vren::model const& model);

	public:
		pre_upload_model specialize(vren::model const& model);
	};
}
