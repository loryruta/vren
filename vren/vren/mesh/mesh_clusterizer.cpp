#include "mesh.hpp"

#include <cassert>
#include <vector>

#include "base/base.hpp"

#undef VREN_LOG_LEVEL
#define VREN_LOG_LEVEL VREN_LOG_LEVEL_WARN
#include "utils/log.hpp"

bool is_triangle_adjacent_to_meshlet(uint32_t const* indices, uint32_t triangle, uint32_t const* meshlet_vertices, vren::meshlet const& meshlet)
{
	uint32_t
		ai = indices[triangle * 3 + 0],
		bi = indices[triangle * 3 + 1],
		ci = indices[triangle * 3 + 2];

	for (uint32_t i = 0; i < meshlet.m_vertex_count; i++)
	{
		if (
			meshlet_vertices[meshlet.m_vertex_offset + i] == ai ||
			meshlet_vertices[meshlet.m_vertex_offset + i] == bi ||
			meshlet_vertices[meshlet.m_vertex_offset + i] == ci
		) {
			return true;
		}
	}

	return false;
}

void get_triangle_vertices(float const* vertices, size_t vertex_stride, uint32_t const* indices, uint32_t triangle_idx, glm::vec3& p0, glm::vec3& p1, glm::vec3& p2)
{
	p0 = *reinterpret_cast<glm::vec3 const*>(&vertices[indices[triangle_idx * 3 + 0] * vertex_stride]);
	p1 = *reinterpret_cast<glm::vec3 const*>(&vertices[indices[triangle_idx * 3 + 1] * vertex_stride]);
	p2 = *reinterpret_cast<glm::vec3 const*>(&vertices[indices[triangle_idx * 3 + 2] * vertex_stride]);
}

vren::bounding_sphere build_triangle_bounding_sphere(float const* vertices, size_t vertex_stride, uint32_t const* indices, uint32_t triangle_idx)
{
	glm::vec3 p0, p1, p2;
	get_triangle_vertices(vertices, vertex_stride, indices, triangle_idx, p0, p1, p2);

	glm::vec3 midpoint = (p0 + p1 + p2) / glm::vec3(3.0f);

	return {
		.m_center = midpoint,
		.m_radius = glm::max(glm::length(p0 - midpoint), glm::max(glm::length(p1 - midpoint), glm::length(p2 - midpoint)))
	};
}

bool does_meshlet_contain_triangle(float const* vertices, size_t vertex_stride, uint32_t const* indices, uint32_t triangle_idx, vren::meshlet& meshlet)
{
	glm::vec3 p0, p1, p2;
	get_triangle_vertices(vertices, vertex_stride, indices, triangle_idx, p0, p1, p2);

	return
		meshlet.m_bounding_sphere.contains(p0) && // Check whether the triangle is fully contained within the meshlet bounding sphere
		meshlet.m_bounding_sphere.contains(p1) &&
		meshlet.m_bounding_sphere.contains(p2);
}

uint8_t get_picked_vertex_index(uint32_t vertex, uint32_t const* meshlet_vertices, vren::meshlet const& meshlet)
{
	uint8_t already_picked = UINT8_MAX; // Return UINT8_MAX if the given vertex wasn't picked yet
	for (uint8_t i = 0; i < meshlet.m_vertex_count; i++) {
		if (meshlet_vertices[meshlet.m_vertex_offset + i] == vertex) {
			already_picked = i;
			break;
		}
	}
	return already_picked;
}

uint32_t pick_vertex(uint32_t vertex, uint32_t* meshlet_vertices, vren::meshlet& meshlet)
{
	meshlet_vertices[meshlet.m_vertex_offset + meshlet.m_vertex_count] = vertex;
	return meshlet.m_vertex_count++;
}

bool pick_triangle(float const* vertices, size_t vertex_stride, uint32_t const* indices, uint32_t triangle_idx, uint32_t* meshlet_vertices, uint8_t* meshlet_triangles, vren::meshlet& meshlet)
{
	if (meshlet.m_triangle_count >= vren::k_max_meshlet_primitive_count) {
		return false;
	}

	uint32_t
		a = indices[triangle_idx * 3 + 0],
		b = indices[triangle_idx * 3 + 1],
		c = indices[triangle_idx * 3 + 2];

	uint8_t ai, bi, ci;
	ai = get_picked_vertex_index(a, meshlet_vertices, meshlet);
	bi = get_picked_vertex_index(b, meshlet_vertices, meshlet);
	ci = get_picked_vertex_index(c, meshlet_vertices, meshlet);

	if (meshlet.m_vertex_count + (ai == UINT8_MAX) + (bi == UINT8_MAX) + (ci == UINT8_MAX) >= vren::k_max_meshlet_vertex_count) {
		return false;
	}

	if (ai == UINT8_MAX) {
		ai = pick_vertex(a, meshlet_vertices, meshlet);
	}

	if (bi == UINT8_MAX) {
		bi = pick_vertex(b, meshlet_vertices, meshlet);
	}

	if (ci == UINT8_MAX) {
		ci = pick_vertex(c, meshlet_vertices, meshlet);
	}

	meshlet_triangles[meshlet.m_triangle_offset + meshlet.m_triangle_count * 3 + 0] = ai;
	meshlet_triangles[meshlet.m_triangle_offset + meshlet.m_triangle_count * 3 + 1] = bi;
	meshlet_triangles[meshlet.m_triangle_offset + meshlet.m_triangle_count * 3 + 2] = ci;
	meshlet.m_triangle_count++;

	glm::vec3 p0, p1, p2;
	get_triangle_vertices(vertices, vertex_stride, indices, triangle_idx, p0, p1, p2);

	meshlet.m_bounding_sphere += build_triangle_bounding_sphere(vertices, vertex_stride, indices, triangle_idx);

	return true;
}

size_t vren::clusterize_mesh(
	float const* vertices,
	size_t vertex_stride,
	uint32_t const* indices,
	size_t index_count,
	uint32_t* meshlet_vertices,
	uint8_t* meshlet_triangles,
	vren::meshlet* meshlets
)
{
	assert(index_count % 3 == 0);

	size_t triangle_count = index_count / 3;

	std::vector<glm::vec3> triangle_midpoints(triangle_count); // A vector holding the midpoints of all triangles
	for (uint32_t triangle = 0; triangle < triangle_count; triangle++)
	{
		glm::vec3 p0, p1, p2;
		get_triangle_vertices(vertices, vertex_stride, indices, triangle, p0, p1, p2);

		glm::vec3 midpoint = (p0 + p1 + p2) / glm::vec3(3.0f);
		triangle_midpoints[triangle] = midpoint;
	}

	std::vector<uint32_t> kd_tree_triangle_indices(triangle_count); // Temporary vector used to build the KD-tree
	for (uint32_t i = 0; i < triangle_count; i++)
	{
		kd_tree_triangle_indices[i] = i;
	}

	std::vector<kd_tree_node> kd_tree(triangle_count * 2); // TODO max size?
	vren::kd_tree_build(reinterpret_cast<float const*>(triangle_midpoints.data()), sizeof(glm::vec3) / sizeof(float), kd_tree_triangle_indices.data(), kd_tree_triangle_indices.size(), kd_tree.data(), 0, 8);

	// 0 = triangle not picked
	// 1 = triangle picked and put in nearby queue
	// 2 = triangle picked by the meshlet
	std::vector<uint8_t> triangle_picked(triangle_count, 0);

	uint32_t nearby_triangle_queue[vren::k_max_meshlet_primitive_count];

	size_t meshlet_count = 0;
	for (uint32_t triangle = 0; triangle < triangle_count; triangle++)
	{
		assert(triangle_picked[triangle] != 1);

		if (triangle_picked.at(triangle)) { // We're searching for the first non-picked triangle
			continue;
		}

		vren::meshlet& meshlet = meshlets[meshlet_count];
		meshlet.m_vertex_offset = meshlet_count > 0 ? meshlets[meshlet_count - 1].m_vertex_offset + meshlets[meshlet_count - 1].m_vertex_count : 0;
		meshlet.m_vertex_count = 0;
		meshlet.m_triangle_offset = meshlet_count > 0 ? meshlets[meshlet_count - 1].m_triangle_offset + meshlets[meshlet_count - 1].m_triangle_count * 3 : 0;
		meshlet.m_triangle_count = 0;
		meshlet.m_bounding_sphere = {}; // Degenerate bounding sphere (radius = 0)

		// PICK FIRST NON-PICKED TRIANGLE
		assert(pick_triangle(vertices, vertex_stride, indices, triangle, meshlet_vertices, meshlet_triangles, meshlet));
		triangle_picked[triangle] = 2;

		VREN_DEBUG("Initialized meshlet {} at triangle {}\n", meshlet_count, triangle);

		// SEARCH FOR NEARBY TRIANGLES
		glm::vec3 sample = triangle_midpoints[triangle];

		uint32_t nearby_triangle_count = 0;
		for (; nearby_triangle_count < std::size(nearby_triangle_queue); nearby_triangle_count++)
		{
			float nearest_triangle_distance = std::numeric_limits<float>::infinity();
			uint32_t nearest_triangle = UINT32_MAX;

			// Search for the nearest non-picked triangle using the built KD-tree
			auto can_pick = [&](uint32_t triangle) -> bool { return triangle_picked[triangle] == 0; };
			kd_tree_search(
				reinterpret_cast<float const*>(triangle_midpoints.data()),
				sizeof(glm::vec3) / sizeof(float),
				kd_tree_triangle_indices.data(),
				kd_tree_triangle_indices.size(),
				kd_tree.data(),
				0,
				(float const*) &sample,
				can_pick,
				nearest_triangle,
				nearest_triangle_distance
			);

			if (nearest_triangle < UINT32_MAX)
			{
				nearby_triangle_queue[nearby_triangle_count] = nearest_triangle;
				triangle_picked[nearest_triangle] = 1;
			}
			else
			{
				break;
			}
		}

		size_t
			picked_for_adjacency_count = 0,
			picked_for_bounding_sphere_count = 0;
		while (true)
		{
			size_t picked_triangle_count;

			// PICK ADJACENT TRIANGLES
			while (true)
			{
				picked_triangle_count = 0;
				for (uint32_t i = 0; i < nearby_triangle_count; i++)
				{
					uint32_t nearby_triangle = nearby_triangle_queue[i];
					if (triangle_picked[nearby_triangle] != 2 && is_triangle_adjacent_to_meshlet(indices, nearby_triangle, meshlet_vertices, meshlet))
					{
						if (pick_triangle(vertices, vertex_stride, indices, nearby_triangle, meshlet_vertices, meshlet_triangles, meshlet))
						{
							triangle_picked[nearby_triangle] = 2;
							picked_triangle_count++;
						}
					}
				}
				picked_for_adjacency_count += picked_triangle_count;
				if (picked_triangle_count == 0) {
					break; // Exit when there's no more adjacent triangle or the meshlet is completed
				}
			}

			// PICK TRIANGLES CONTAINED IN MESHLET BOUNDING-SPHERE
			picked_triangle_count = 0;
			for (uint32_t i = 0; i < nearby_triangle_count; i++)
			{
				uint32_t nearby_triangle = nearby_triangle_queue[i];
				if (triangle_picked[nearby_triangle] != 2 && does_meshlet_contain_triangle(vertices, vertex_stride, indices, nearby_triangle, meshlet))
				{
					if (pick_triangle(vertices, vertex_stride, indices, nearby_triangle, meshlet_vertices, meshlet_triangles, meshlet))
					{
						triangle_picked[nearby_triangle] = 2;
						picked_triangle_count++;
					}
				}
			}
			picked_for_bounding_sphere_count += picked_triangle_count;
			if (picked_triangle_count == 0) { // No adjacent triangle and no triangle is contained in the meshlet bounding sphere, there's no more work
				break;
			}
		}

		VREN_DEBUG("Nearby triangle count: {}, Picked for bounding sphere {}, Picked for adjacency {}\n", nearby_triangle_count, picked_for_bounding_sphere_count, picked_for_adjacency_count);

		// EMPTY QUEUE
		for (uint32_t i = 0; i < nearby_triangle_count; i++)
		{
			uint32_t nearby_triangle = nearby_triangle_queue[i];
			if (triangle_picked[nearby_triangle] == 1) {
				triangle_picked[nearby_triangle] = 0;
			}
		}

		//VREN_DEBUG("Meshlet {} completed - {} vertices, {} triangles\n", meshlet_count, meshlet.m_vertex_count, meshlet.m_triangle_count);

		// MESHLET COMPLETED
		meshlet_count++;

		VREN_DEBUG("> Meshlet completed - vertices: {}, triangles: {}\n", meshlet.m_vertex_count, meshlet.m_triangle_count);
	}

	return meshlet_count;
}

