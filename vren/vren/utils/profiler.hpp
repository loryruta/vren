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
		static constexpr uint32_t k_max_slot_count = 256;

	private:
		vren::context const* m_context;
		vren::vk_query_pool m_query_pool;

	public:
		profiler(vren::context const& context);
		~profiler();

	private:
		vren::vk_query_pool create_query_pool() const;

	public:
		vren::render_graph::node* profile(vren::render_graph::allocator& allocator, uint32_t slot_idx, vren::render_graph::graph_t const& sample);

		bool read_timestamps(uint32_t slot_idx, uint64_t& start_timestamp, uint64_t& end_timestamp);

		inline uint64_t read_elapsed_time(uint32_t slot_idx)
		{
			uint64_t start_timestamp, end_timestamp;
			if (read_timestamps(slot_idx, start_timestamp, end_timestamp)) {
				return end_timestamp - start_timestamp;
			} else {
				return -1;
			}
		}
	};
}
