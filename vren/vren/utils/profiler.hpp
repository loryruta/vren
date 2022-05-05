#pragma once

#include <unordered_map>
#include <bitset>

#include "base/resource_container.hpp"
#include "vk_helpers/vk_raii.hpp"
#include "render_graph.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Profiler
	// ------------------------------------------------------------------------------------------------

	class profiler
	{
	public:
		static constexpr uint32_t k_max_slot_count = 32;

	private:
		vren::context const* m_context;
		vren::vk_query_pool m_query_pool;

		std::unordered_map<std::string, uint32_t> m_slot_by_name;
		std::bitset<k_max_slot_count> m_slot_used;

	public:
		profiler(vren::context const& context);
		~profiler();

	private:
		vren::vk_query_pool create_query_pool() const;

	public:
		vren::render_graph::node* profile(
			vren::render_graph::allocator& allocator,
			std::string const& slot_name,
			vren::render_graph::graph_t const& sample
		);

		bool pull_slot_timestamps(std::string const& slot_name, uint64_t* start_t, uint64_t* end_t);
	};
}
