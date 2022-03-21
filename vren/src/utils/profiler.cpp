#include "profiler.hpp"

#include "pooling/command_pool.hpp"
#include "utils/misc.hpp"

vren::profiler::profiler(std::shared_ptr<vren::context> const& ctx, size_t slots_count) :
	m_context(ctx),
	m_query_pool(vren::vk_utils::create_timestamp_query_pool(ctx, slots_count * 2))
{}

vren::profiler::~profiler()
{}

void vren::profiler::profile(
	int slot_idx,
	vren::command_graph& cmd_graph,
	vren::resource_container& res_container,
	std::function<void()> const& sample_func
)
{
	{ /* Starting timestamp */
		auto& begin_node = cmd_graph.create_tail_node();
		vren::vk_utils::record_one_time_submit_commands(begin_node.m_command_buffer.m_handle, [&](VkCommandBuffer cmd_buf)
		{
			vkCmdResetQueryPool(cmd_buf, m_query_pool.m_handle, slot_idx * 2, 2);

			vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 0, nullptr);

			auto query_idx = slot_idx * 2;
			vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, query_idx);
		});
	}

	/* Recording */
	sample_func();

	{ /* Ending timestamp */
		auto& end_node = cmd_graph.create_tail_node();
		vren::vk_utils::record_one_time_submit_commands(end_node.m_command_buffer.m_handle, [&](VkCommandBuffer cmd_buf)
		{
			auto query_idx = slot_idx * 2 + 1;
			vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, query_idx);
		});
	}
}

bool vren::profiler::get_timestamps(int slot_idx, uint64_t* start_t, uint64_t* end_t)
{
	uint64_t buf[2];
	VkResult res = vkGetQueryPoolResults(m_context->m_device, m_query_pool.m_handle, slot_idx * 2, 2, sizeof(uint64_t) * 2, &buf[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

	// TODO USE IT (assumed 1): auto t_period = m_context->m_physical_device_properties.limits.timestampPeriod;
	// TODO USE IT (assumed 64): auto t_valid_bits = vren::vk_utils::get_queue_families_properties(m_context->m_physical_device).at(m_context->m_queue_families.m_graphics_idx).timestampValidBits;

	if (res == VK_NOT_READY)
	{
		return false;
	}
	else if (res == VK_SUCCESS)
	{
		*start_t = buf[0];
		*end_t   = buf[1];
		return true;
	}
	else
	{
		throw std::runtime_error("Failed to read query results");
	}
}

