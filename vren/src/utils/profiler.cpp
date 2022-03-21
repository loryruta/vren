#include "profiler.hpp"

#include "pooling/command_pool.hpp"
#include "utils/misc.hpp"

vren::profiler::profiler(std::shared_ptr<vren::context> const& ctx, size_t slots_count) :
	m_query_pool(vren::vk_utils::create_timestamp_query_pool(ctx, slots_count * 2))
{}

vren::profiler::~profiler()
{}

void vren::profiler::profile(
	int slot_idx,
	vren::resource_container& res_container,
	vren::command_graph& cmd_graph,
	std::function<void()> const& sample_func
)
{
	{ /* Starting timestamp */
		auto& begin_node = cmd_graph.create_tail_node();
		VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr
		};
		vkBeginCommandBuffer(begin_node.m_command_buffer.m_handle, &begin_info);
		vkCmdWriteTimestamp(begin_node.m_command_buffer.m_handle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_query_pool.m_handle, slot_idx * 2);
		vkEndCommandBuffer(begin_node.m_command_buffer.m_handle);
	}

	/* Recording */
	sample_func();

	{ /* Ending timestamp */
		auto& end_node = cmd_graph.create_tail_node();
		VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr
		};
		vkBeginCommandBuffer(end_node.m_command_buffer.m_handle, &begin_info);
		vkCmdWriteTimestamp(end_node.m_command_buffer.m_handle, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_query_pool.m_handle, slot_idx * 2 + 1);
		vkEndCommandBuffer(end_node.m_command_buffer.m_handle);
	}
}

bool vren::profiler::get_timestamps(int slot_idx, uint64_t* starting_timestamp, uint64_t* ending_timestamp)
{
	uint64_t buf[2];

	VkResult res = vkGetQueryPoolResults(m_context->m_device, m_query_pool.m_handle, slot_idx * 2, 2, sizeof(uint64_t) * 2, buf, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

	vkResetQueryPool(m_context->m_device, m_query_pool.m_handle, slot_idx * 2, 2);

	if (res == VK_NOT_READY)
	{
		return false;
	}
	else if (res == VK_SUCCESS)
	{
		*starting_timestamp = buf[0];
		*ending_timestamp = buf[1];
		return true;
	}
	else
	{
		throw std::runtime_error("Failed to read query results");
	}
}

