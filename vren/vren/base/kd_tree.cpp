#include "kd_tree.hpp"

#include <cassert>

size_t vren::kd_tree_build(
	float const* points,
	size_t point_stride,
	uint32_t* indices,
	size_t count,
	kd_tree_node* kd_tree,
	size_t node_offset,
	size_t max_leaf_point_count
)
{
	assert(count > 0);

	if (count <= max_leaf_point_count)
	{
		kd_tree_node& first_leaf_node = kd_tree[node_offset];
		first_leaf_node.m_index = indices[0];
		first_leaf_node.m_axis = 0x3;
		first_leaf_node.m_leaf_count = count;

		for (uint32_t i = 1; i < count; i++)
		{
			kd_tree_node& leaf_node = kd_tree[node_offset + i];
			leaf_node.m_index = indices[i];
			leaf_node.m_axis = 0x3;
			leaf_node.m_leaf_count = ~0;
		}
		return node_offset + count;
	}

	float mean[3] = {};
	float variance[3] = {};
	float n = 1.0f;

	for (uint32_t i = 0; i < count; i++, n += 1.0f)
	{
		float const* point = points + indices[i] * point_stride;
		for (uint32_t j = 0; j < 3; j++)
		{
			float d = point[j] - mean[j];
			mean[j] += d / n;
			variance[j] += (point[j] - mean[j]) * d; // Welford algorithm for computing variance
		}
	}

	// variance[j] /= float(point_count - 1)
	// We don't need to divide since we don't need the exact variance value, we just have to compare the greatest axis

	uint32_t axis = variance[0] > variance[1] && variance[0] > variance[1] ? 0 : (variance[1] > variance[2] ? 1 : 2);
	float split = mean[axis];

	size_t middle = 0;
	for (uint32_t i = 0; i < count; i++)
	{
		float const* point = points + indices[i] * point_stride;

		if (point[axis] < split)
		{
			uint32_t tmp = indices[i];
			indices[i] = indices[middle];
			indices[middle] = tmp;

			middle++;
		}
	}

	size_t next_node_offset = kd_tree_build(points, point_stride, indices, middle, kd_tree, node_offset + 1, max_leaf_point_count);

	kd_tree_node& node = kd_tree[node_offset];
	node.m_split = split;
	node.m_axis = axis;
	node.m_right_child_distance = next_node_offset - node_offset;

	return kd_tree_build(points, point_stride, indices + middle, count - middle, kd_tree, next_node_offset, max_leaf_point_count);
}

void vren::kd_tree_search(
	float const* points,
	size_t point_stride,
	uint32_t const* indices,
	size_t count,
	kd_tree_node* kd_tree,
	size_t node_offset,
	float const* sample,
	vren::kd_tree_search_filter_t const& filter_predicate,
	uint32_t& best_point,
	float& best_distance_squared
)
{
	kd_tree_node& node = kd_tree[node_offset];

	if (node.m_axis == 0x3) // Is children
	{
		for (uint32_t i = 0; i < node.m_leaf_count; i++)
		{
			kd_tree_node& leaf_node = kd_tree[node_offset + i];

			if (!filter_predicate(leaf_node.m_index)) {
				continue;
			}

			float const* point = points + leaf_node.m_index * point_stride;
			float distance_squared =
				(sample[0] - point[0]) * (sample[0] - point[0]) +
				(sample[1] - point[1]) * (sample[1] - point[1]) +
				(sample[2] - point[2]) * (sample[2] - point[2]);
			if (distance_squared < best_distance_squared)
			{
				best_point = leaf_node.m_index;
				best_distance_squared = distance_squared;
			}
		}
	}
	else
	{
		float d = sample[node.m_axis] - node.m_split;
		size_t first_node_offset  = d <= 0 ? node_offset + 1 : node_offset + node.m_right_child_distance;
		size_t second_node_offset = d <= 0 ? node_offset + node.m_right_child_distance : node_offset + 1;

		kd_tree_search(points, point_stride, indices, count, kd_tree, first_node_offset, sample, filter_predicate, best_point, best_distance_squared);
		if (best_distance_squared > (d * d))
		{
			kd_tree_search(points, point_stride, indices, count, kd_tree, second_node_offset, sample, filter_predicate, best_point, best_distance_squared);
		}
	}
}


