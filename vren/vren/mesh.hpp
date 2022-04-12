#pragma once

#include <vector>

namespace vren
{
	struct meshlet
	{
		size_t m_index_offset;
		size_t m_index_count;
	};

	void reorder_vertex_buffer(float* vertices, size_t vertex_count);

	void create_meshlets(uint32_t* indices, size_t index_count, std::vector<meshlet>& meshlets);
}

