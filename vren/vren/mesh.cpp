#include "mesh.hpp"

#include <cassert>
#include <vector>

void vren::reorder_vertex_buffer(float* vertices, size_t vertex_count)
{

}

void vren::create_meshlets(uint32_t* indices, size_t index_count, std::vector<meshlet>& meshlets)
{
	assert(index_count % 3 == 0);

	const size_t k_max_vertices = 64;
	const size_t k_max_primitives = 124;

	size_t prim_off = 0, prim_count = 0;
	size_t vtx_count = 0;

	for (size_t i = 0; i < index_count; i++)
	{
		prim_off++;
		prim_count++;

		bool has_used_vertex = false;
		for (size_t j = prim_off; j < i; j++) {
			if (indices[i] == indices[j]) {
				has_used_vertex = true;
			}
		}
		if (has_used_vertex) {
			vtx_count++;
		}

		if (prim_count >= k_max_primitives || vtx_count >= k_max_vertices)
		{
			meshlets.push_back({
			    .m_index_offset = prim_off,
				.m_index_count = prim_count
		   });

			prim_off = 0;
			prim_count = 0;
		}
	}
}
