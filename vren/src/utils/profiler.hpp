#pragma once

#include <functional>

#include "context.hpp"
#include "resource_container.hpp"
#include "utils/vk_raii.hpp"
#include "command_graph.hpp"

namespace vren
{
	struct profiler
	{
	private:
		std::shared_ptr<vren::context> m_context;
		vren::vk_query_pool m_query_pool;

	public:
		profiler(std::shared_ptr<vren::context> const& ctx, size_t slots_count);
		~profiler();

		void profile(
			int slot_idx,
			vren::resource_container& res_container,
			vren::command_graph& cmd_graph,
			std::function<void()> const& sample_func
		);

		bool get_timestamps(
			int slot_idx,
			uint64_t* starting_timestamp,
			uint64_t* ending_timestamp
		);
	};

}
