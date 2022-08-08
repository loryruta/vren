#pragma once

#include <functional>

#include <glm/glm.hpp>

#include "fixed_capacity_vector.hpp"

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
		return value > 0 && (value & (value - 1)) == 0;
	}

	inline uint32_t round_to_next_power_of_2(uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	inline uint32_t round_to_nearest_multiple_of_power_of_2(uint32_t value, uint32_t power_of_2)
	{
		assert(power_of_2 > 0 && is_power_of_2(power_of_2));
		return (value + power_of_2 - 1) & ~(power_of_2 - 1);
	}

	template<typename _integer_t>
	_integer_t round_to_next_multiple_of(_integer_t value, _integer_t multiple)
	{
		_integer_t reminder = value % multiple;
		return reminder == 0 ? value : (value + multiple - reminder);
	}

	template<typename _integer_t>
	bool is_power_of(_integer_t n, _integer_t base)
	{
		double exp = glm::log((double) n) / glm::log((double) base);
		return (exp - glm::floor(exp)) < std::numeric_limits<double>::epsilon();
	}

	template<typename _integer_t>
	_integer_t round_to_next_power_of(_integer_t n, _integer_t base)
	{
		double exp = glm::log((double) n) / glm::log((double) base);
		return (uint32_t) glm::pow(base, glm::ceil(exp));
	}

	inline uint32_t divide_and_ceil(uint32_t value, uint32_t divider)
	{
		return (uint32_t) std::ceil(double(value) / double(divider));
	}

	template<typename _iterator_t, typename _predicate_t>
	decltype(auto) find_if_or_fail_const(_iterator_t begin, _iterator_t end, _predicate_t predicate)
	{
		auto found = std::find_if(begin, end, predicate);
		if (found == end)
		{
			throw std::runtime_error("No element satisfying the given predicate");
		}
		return *found;
	}

	// ------------------------------------------------------------------------------------------------

	template<typename _t, size_t _size>
	using static_vector_t = std::experimental::fixed_capacity_vector<_t, _size>;

	// ------------------------------------------------------------------------------------------------
	// Bounding sphere
	// ------------------------------------------------------------------------------------------------

	struct bounding_sphere
	{
		glm::vec3 m_center;
		float m_radius;

		bool is_degenerate() const;
		bool intersects(bounding_sphere const& other) const;

		bool contains(glm::vec3 const& point) const;
		bool contains(bounding_sphere const& other) const;

		void operator+=(glm::vec3 const& point);
		void operator+=(bounding_sphere const& other);
	};
}

