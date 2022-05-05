#include "profiler.hpp"

#include <stdexcept>

#include "context.hpp"
#include "vk_helpers/misc.hpp"

vren::profiler::profiler(vren::context const& context) :
	m_context(&context),
	m_query_pool(create_query_pool())
{}

vren::profiler::~profiler()
{}

vren::vk_query_pool vren::profiler::create_query_pool() const
{
	VkQueryPoolCreateInfo query_pool_info{
		.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.queryType = VK_QUERY_TYPE_TIMESTAMP,
		.queryCount = k_max_slot_count * 2,
		.pipelineStatistics = NULL
	};

	VkQueryPool query_pool;
	VREN_CHECK(vkCreateQueryPool(m_context->m_device, &query_pool_info, nullptr, &query_pool), m_context);
	return vren::vk_query_pool(*m_context, query_pool);
}

vren::render_graph::node* vren::profiler::profile(vren::render_graph::allocator& allocator, uint32_t slot_idx, vren::render_graph::graph_t const& sample)
{
	// Starting time
	auto head_node = allocator.allocate();
	head_node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		vkCmdResetQueryPool(command_buffer, m_query_pool.m_handle, slot_idx * 2, 2);
		vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 0);
	});

	vren::render_graph::iterate_starting_nodes(allocator, sample, [&](vren::render_graph::node* node) {
		head_node->add_next(node);
	});

	// Ending time
	auto tail_node = allocator.allocate();
	tail_node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 1);
	});

	vren::render_graph::iterate_ending_nodes(allocator, sample, [&](vren::render_graph::node* node) {
		node->add_next(tail_node);
	});

	return head_node;
}

bool vren::profiler::read_timestamps(uint32_t slot_idx, uint64_t& start_timestamp, uint64_t& end_timestamp)
{
	uint64_t timestamps[2];
	VkResult result = vkGetQueryPoolResults(m_context->m_device, m_query_pool.m_handle, slot_idx * 2, 2, sizeof(uint64_t) * 2, &timestamps[0], sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

	// TODO USE IT (assumed 1): auto t_period = m_context->m_physical_device_properties.limits.timestampPeriod;
	// TODO USE IT (assumed 64): auto t_valid_bits = vren::vk_utils::get_queue_families_properties(m_context->m_physical_device).at(m_context->m_queue_families.m_graphics_idx).timestampValidBits;

	if (result == VK_NOT_READY)
	{
		return false;
	}
	else if (result == VK_SUCCESS)
	{
		start_timestamp = timestamps[0];
		end_timestamp = timestamps[1];
		return true;
	}
	else
	{
		throw std::runtime_error("Failed to read_elapsed_time query results");
	}
}
