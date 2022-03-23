#include "command_graph.hpp"

#include "utils/misc.hpp"

// ------------------------------------------------------------------------------------------------
// command_graph_node
// ------------------------------------------------------------------------------------------------

void vren::command_graph_node::add_src_dependency(command_graph_node& other)
{
	auto& sem = m_command_graph.lock()->acquire_semaphore();
	m_src_semaphores.push_back(sem.m_handle);
	other.m_dst_semaphores.push_back(sem.m_handle);
}

void vren::command_graph_node::add_dst_dependency(command_graph_node& other)
{
	other.add_src_dependency(*this);
}

// ------------------------------------------------------------------------------------------------
// command_graph
// ------------------------------------------------------------------------------------------------

vren::command_graph::command_graph(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<vren::command_pool> const& cmd_pool
) :
	m_context(ctx),
	m_command_pool(cmd_pool),

	m_src_semaphore(vren::vk_utils::create_semaphore(ctx)),
	m_dst_semaphore(vren::vk_utils::create_semaphore(ctx))
{}

vren::vk_semaphore& vren::command_graph::acquire_semaphore()
{
	return m_acquired_semaphores.emplace_back(vren::vk_utils::create_semaphore(m_context));
}

vren::command_graph_node& vren::command_graph::create_node()
{
	auto node_idx = m_nodes.size();
	return m_nodes.emplace_back(shared_from_this(), node_idx, m_command_pool->acquire());
}

vren::command_graph_node& vren::command_graph::create_tail_node()
{
	auto& node = create_node();
	for (auto& other_node : m_nodes) {
		if (other_node.m_index != node.m_index && other_node.m_dst_semaphores.empty()) {
			node.add_src_dependency(other_node);
		}
	}
	return node;
}

void vren::command_graph::submit(
	VkQueue queue,
	VkFence dst_fence,
	size_t src_semaphores_count,
	VkSemaphore* src_semaphores,
	size_t dst_semaphores_count,
	VkSemaphore* dst_semaphores
)
{
	std::vector<VkSemaphoreWaitFlags> wait_stages{};

	std::vector<VkSubmitInfo> submits{};

	/* Pre-graph semaphores */
	if (src_semaphores_count > 0)
	{
		if (src_semaphores_count > wait_stages.size()) {
			wait_stages.resize(src_semaphores_count, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}

		submits.push_back({
			  .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			  .pNext = nullptr,
			  .waitSemaphoreCount = (uint32_t) src_semaphores_count,
			  .pWaitSemaphores = src_semaphores,
			  .pWaitDstStageMask = wait_stages.data(),
			  .commandBufferCount = 0,
			  .pCommandBuffers = nullptr,
			  .signalSemaphoreCount = 1,
			  .pSignalSemaphores = &m_src_semaphore.m_handle
		  });
	}

	/* Graph */
	for (auto& node : m_nodes)
	{
		size_t node_src_semaphores_count = node.m_src_semaphores.empty() && src_semaphores_count > 0 ? 1                         : node.m_src_semaphores.size();
		VkSemaphore* node_src_semaphores = node.m_src_semaphores.empty() && src_semaphores_count > 0 ? &m_src_semaphore.m_handle : node.m_src_semaphores.data();

		size_t node_dst_semaphores_count = node.m_dst_semaphores.empty() && dst_semaphores_count > 0 ? 1                         : node.m_dst_semaphores.size();
		VkSemaphore* node_dst_semaphores = node.m_dst_semaphores.empty() && dst_semaphores_count > 0 ? &m_dst_semaphore.m_handle : node.m_dst_semaphores.data();

		if (node_src_semaphores_count > wait_stages.size()) {
			wait_stages.resize(node_src_semaphores_count, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}

		submits.push_back({
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = (uint32_t) node_src_semaphores_count,
			.pWaitSemaphores = node_src_semaphores,
			.pWaitDstStageMask = wait_stages.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &node.m_command_buffer.get(),
			.signalSemaphoreCount = (uint32_t) node_dst_semaphores_count,
			.pSignalSemaphores = node_dst_semaphores
		});
	}

	/* Post-graph semaphores */
	if (dst_semaphores_count > 0)
	{
		if (1 > wait_stages.size()) {
			wait_stages.resize(1, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		}

		submits.push_back({
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &m_dst_semaphore.m_handle,
			.pWaitDstStageMask = wait_stages.data(),
			.commandBufferCount = 0,
			.pCommandBuffers = nullptr,
			.signalSemaphoreCount = (uint32_t) dst_semaphores_count,
			.pSignalSemaphores = dst_semaphores
		});
	}

	vkQueueSubmit(queue, submits.size(), submits.data(), dst_fence);
}
