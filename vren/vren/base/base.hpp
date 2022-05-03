#pragma once

#include <functional>

#include <glm/glm.hpp>

namespace vren
{
	namespace detail
	{
		template<typename _t, size_t... _is>
		auto create_array(std::function<_t(uint32_t idx)> const& create_entry_func, std::index_sequence<_is...> _)
		{
			return std::array{ (_is, create_entry_func(_is))... };
		}
	}

	template<typename _t, size_t _size>
	auto create_array(std::function<_t(uint32_t idx)> const& create_entry_func)
	{
		return detail::create_array(create_entry_func, std::make_index_sequence<_size>());
	}

	template<typename _t, size_t _size>
	auto create_array(_t value)
	{
		return create_array<_t, _size>([&](uint32_t idx) { return value; });
	}

	inline constexpr bool is_power_of_2(uint32_t value)
	{
		return (value & (value - 1)) == 0;
	}

	inline uint32_t round_to_nearest_multiple_of_power_of_2(uint32_t value, uint32_t power_of_2)
	{
		assert(power_of_2 > 0 && is_power_of_2(power_of_2));
		return (value + power_of_2 - 1) & ~(power_of_2 - 1);
	}

	// ------------------------------------------------------------------------------------------------
	// Bounding sphere
	// ------------------------------------------------------------------------------------------------

	struct bounding_sphere
	{
		glm::vec3 m_center;
		float m_radius;
		float _pad[2];

		bool is_degenerate() const;
		bool intersects(bounding_sphere const& other) const;

		bool contains(glm::vec3 const& point) const;
		bool contains(bounding_sphere const& other) const;

		void operator+=(glm::vec3 const& point);
		void operator+=(bounding_sphere const& other);
	};

	// ------------------------------------------------------------------------------------------------
	// KD-tree
	// ------------------------------------------------------------------------------------------------

	struct kd_tree_node
	{
		union { float m_split; uint32_t m_index; };

		uint32_t m_axis: 2;
		union { uint32_t m_right_child_distance: 30; uint32_t m_leaf_count: 30; };
	};

	size_t kd_tree_build(
		float const* points,
		size_t point_stride,
		uint32_t* indices,
		size_t count,
		kd_tree_node* kd_tree,
		size_t node_offset,
		size_t max_leaf_point_count
	);

	using kd_tree_search_filter_t = std::function<bool(uint32_t point)>;

	const kd_tree_search_filter_t k_kd_tree_default_search_filter = [](uint32_t point) -> bool { return true; };

	void kd_tree_search(
		float const* points,
		size_t point_stride,
		uint32_t const* indices,
		size_t count,
		kd_tree_node* kd_tree,
		size_t node_offset,
		float const* sample,
		kd_tree_search_filter_t const& filter_predicate,
		uint32_t& best_point,
		float& best_distance_squared
	);
}

