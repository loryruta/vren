#pragma once

#include <functional>

#include "base/resource_container.hpp"
#include "vk_helpers/vk_raii.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Profiler
	// ------------------------------------------------------------------------------------------------

	class profiler
	{
	private:
		vren::context const* m_context;
		vren::vk_query_pool m_query_pool;

	public:
		profiler(vren::context const& ctx, size_t slots_count);
		~profiler();

		void profile(
			int slot_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			std::function<void()> const& sample_func
		);

		bool get_timestamps(int slot_idx, uint64_t* start_t, uint64_t* end_t);
	};

}
