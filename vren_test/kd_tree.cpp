#include <gtest/gtest.h>

#include <random>
#include <chrono>

#include <vren/base/base.hpp>

void linear_nearest_neighbour_search(float const* points, size_t point_stride, uint32_t* indices, size_t count, float const* sample, uint32_t& best_point, float& best_distance_squared)
{
	best_distance_squared = std::numeric_limits<float>::infinity();
	best_point = UINT32_MAX;

	for (uint32_t i = 0; i < count; i++)
	{
		float const* point = &points[indices[i] * point_stride];
		float distance_squared =
			(sample[0] - point[0]) * (sample[0] - point[0]) +
			(sample[1] - point[1]) * (sample[1] - point[1]) +
			(sample[2] - point[2]) * (sample[2] - point[2]);
		if (distance_squared < best_distance_squared)
		{
			best_distance_squared = distance_squared;
			best_point = indices[i];
		}
	}
}

TEST(KDTree, NearestNeigborSearch)
{
	std::random_device random_dev;
	std::mt19937 rng(random_dev());

	const float k_aabb_min = 0.0f;
	const float k_aabb_max = 100.0f;
	const size_t k_max_point_count = 1'000'000;
	const size_t k_sample_count = 100;
	const size_t k_point_stride = 3;

	std::uniform_real_distribution<> random_distribution(k_aabb_min, k_aabb_max);

	// Points
	std::vector<float> points(k_max_point_count * k_point_stride);
	for (uint32_t i = 0; i < k_max_point_count; i++)
	{
		points[i * k_point_stride + 0] = random_distribution(rng);
		points[i * k_point_stride + 1] = random_distribution(rng);
		points[i * k_point_stride + 2] = random_distribution(rng);
	}

	// Indices
	std::vector<uint32_t> indices(k_max_point_count);
	for (uint32_t i = 0; i < k_max_point_count; i++)
	{
		indices[i] = i;
	}

	// Build KD-tree
	std::vector<vren::kd_tree_node> kd_tree(k_max_point_count * 2);
	vren::kd_tree_build(points.data(), k_point_stride, indices.data(), indices.size(), kd_tree.data(), 0, 8);

	// Samples
	std::vector<float> samples(k_sample_count * k_point_stride);
	for (uint32_t i = 0; i < k_sample_count; i++)
	{
		samples[i * k_point_stride + 0] = random_distribution(rng);
		samples[i * k_point_stride + 1] = random_distribution(rng);
		samples[i * k_point_stride + 2] = random_distribution(rng);
	}

	// Searching
	uint64_t elapsed_time_1 = 0, elapsed_time_2 = 0;
	std::chrono::time_point<std::chrono::steady_clock> start_at;

	for (uint32_t i = 0; i < k_sample_count; i++)
	{
		float best_distance_1, best_distance_2;
		uint32_t best_point_1, best_point_2;

		// Linear search
		start_at = std::chrono::steady_clock::now();

		linear_nearest_neighbour_search(points.data(), k_point_stride, indices.data(), k_max_point_count, &samples[i * k_point_stride], best_point_1, best_distance_1);

		elapsed_time_1 += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_at).count();

		// KD-tree search
		start_at = std::chrono::steady_clock::now();

		best_distance_2 = std::numeric_limits<float>::infinity();
		best_point_2 = UINT32_MAX;
		vren::kd_tree_search(points.data(), k_point_stride, indices.data(), k_max_point_count, kd_tree.data(), 0, &samples[i * k_point_stride], vren::k_kd_tree_default_search_filter, best_point_2, best_distance_2);

		elapsed_time_2 += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start_at).count();

		// ads asdd s
		EXPECT_EQ(best_distance_1, best_distance_2);
		EXPECT_EQ(best_point_1, best_point_2);
	}

	printf("Linear search - Point count: %lld, Searches: %lld, Elapsed time: %lld ns\n",
		   k_max_point_count,
		   k_sample_count,
		   elapsed_time_1
	);

	printf("KD-tree search - Point count: %lld, Searches: %lld, Elapsed time: %lld ns\n",
		   k_max_point_count,
		   k_sample_count,
		   elapsed_time_2
	);
}
