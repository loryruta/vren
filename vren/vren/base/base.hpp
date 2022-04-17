#pragma once

#include <functional>

namespace vren
{
	namespace detail
	{
		template<typename _t, size_t... _is>
		auto create_array(std::function<_t()> const& create_entry_func, std::index_sequence<_is...> _)
		{
			return std::array{ (_is, create_entry_func())... };
		}
	}

	template<typename _t, size_t _size>
	auto create_array(std::function<_t()> const& create_entry_func)
	{
		return detail::create_array(create_entry_func, std::make_index_sequence<_size>());
	}

	template<typename _t, size_t _size>
	auto create_array(_t value)
	{
		return create_array<_t, _size>([&]() { return value; });
	}
}

