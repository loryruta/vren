#pragma once

#include <cstdint>
#include <cstdio>

namespace vren
{
	struct kdtree_node
	{
		union
		{
			float m_split;
			uint32_t m_index;
		};

		uint32_t m_axis: 2;

		union
		{
			uint32_t m_right_child_distance: 30;
			uint32_t m_leaf_count: 30;
		};
	};

	inline size_t kdtree_build(
		float const* points,
		size_t point_stride,
		uint32_t* indices,
		size_t count,
		kdtree_node* kdtree,
		size_t node_offset,
		size_t max_leaf_point_count
	)
	{
		if (count <= max_leaf_point_count)
		{
			kdtree_node& first_leaf_node = kdtree[node_offset];
			first_leaf_node.m_index = indices[0];
			first_leaf_node.m_axis = 0x3;
			first_leaf_node.m_leaf_count = count;

			for (uint32_t i = 1; i < count; i++)
			{
				kdtree_node& leaf_node = kdtree[node_offset + i];
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

		kdtree_node& node = kdtree[node_offset];
		node.m_split = split;
		node.m_axis = axis;

		size_t next_node_offset = kdtree_build(points, point_stride, indices, middle, kdtree, node_offset + 1, max_leaf_point_count);

		node.m_right_child_distance = next_node_offset - node_offset;

		return kdtree_build(points, point_stride, indices + middle, count - middle, kdtree, next_node_offset, max_leaf_point_count);
	}

	inline void kd_tree_search(
		float const* points,
		size_t point_stride,
		uint32_t* indices,
		size_t count,
		kdtree_node* kdtree,
		size_t node_offset,
		float const* sample,
		uint32_t& best_point,
		float& best_distance_squared
	)
	{
		kdtree_node& node = kdtree[node_offset];

		if (node.m_axis == 0x3)
		{
			for (uint32_t i = 0; i < node.m_leaf_count; i++)
			{
				kdtree_node& leaf_node = kdtree[node_offset + i];
				float const* point = points + leaf_node.m_index * point_stride;
				float distance_squared = sample[0] * point[0] + sample[1] * point[1] + sample[2] * point[2];
				if (distance_squared < best_distance_squared)
				{
					best_point = leaf_node.m_index;
					best_distance_squared = distance_squared;
				}
			}
			return;
		}
		else
		{
			float d = node.m_split - sample[node.m_axis];
			size_t first_node_offset  = d <= 0 ? node_offset + 1 : node_offset + node.m_right_child_distance;
			size_t second_node_offset = d <= 0 ? node_offset + node.m_right_child_distance : node_offset + 1;

			// Test first the partition where the point resides, it's more probable to find a NN there
			kd_tree_search(points, point_stride, indices, count, kdtree, first_node_offset, sample, best_point, best_distance_squared);

			// If the other partition is furthest compared to the nearest distance found we shouldn't visit it
			if (best_distance_squared >= (d * d)) {
				kd_tree_search(points, point_stride, indices, count, kdtree, second_node_offset, sample, best_point, best_distance_squared);
			}
		}

	}
}
