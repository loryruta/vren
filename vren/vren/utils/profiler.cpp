#include "profiler.hpp"

#include <chrono>

#include "context.hpp"
#include "vk_helpers/misc.hpp"

vren::profiler::profiler(vren::context const& ctx, size_t slots_count) :
	m_context(&ctx),
	m_query_pool(vren::vk_utils::create_timestamp_query_pool(ctx, slots_count * 2))
{}

vren::profiler::~profiler()
{}

void vren::profiler::profile(
	uint32_t frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	uint32_t slot_idx,
	std::function<void()> const& sample_func
)
{
	/* Starting timestamp */
	vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 0, nullptr); // Needed ?

	vkCmdResetQueryPool(cmd_buf, m_query_pool.m_handle, slot_idx * 2, 2);
	vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2);

	/* Recording */
	sample_func();

	/* Ending timestamp */
	vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 1);
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

uint32_t vren::profile(std::function<void()> const& sample_func)
{
	auto start = std::chrono::system_clock::now();

	sample_func();

	auto end = std::chrono::system_clock::now();

	return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
}
