#pragma once

#include "utils/vk_raii.hpp"
#include "pooling/command_pool.hpp"
#include "resource_container.hpp"

namespace vren // TODO vk_utils
{
	// Forward decl
	class context;
	class command_graph;
	class command_graph_node;

	// ------------------------------------------------------------------------------------------------
	// command_graph_node
	// ------------------------------------------------------------------------------------------------

	class command_graph_node
	{
	public:
		std::weak_ptr<command_graph> m_command_graph;

		uint32_t m_index;
		vren::pooled_vk_command_buffer m_command_buffer;

		std::vector<VkSemaphore> m_src_semaphores;
		std::vector<VkSemaphore> m_dst_semaphores;

		command_graph_node(
			std::shared_ptr<command_graph> cmd_graph,
			uint32_t idx,
			vren::pooled_vk_command_buffer&& cmd_buf
		) :
			m_command_graph(std::move(cmd_graph)),
			m_index(idx),
			m_command_buffer(std::move(cmd_buf))
		{}

		void add_src_dependency(command_graph_node& other);
		void add_dst_dependency(command_graph_node& other);
	};

	// ------------------------------------------------------------------------------------------------
	// command_graph
	// ------------------------------------------------------------------------------------------------

	class command_graph : public std::enable_shared_from_this<command_graph>
	{
	private:
		std::shared_ptr<vren::context> m_context;
		std::shared_ptr<vren::command_pool> m_command_pool;

		std::vector<command_graph_node> m_nodes;

		std::vector<vren::vk_semaphore> m_acquired_semaphores;
		vren::vk_semaphore m_src_semaphore, m_dst_semaphore;

	public:
		command_graph(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::command_pool> const& cmd_pool
		);

		vren::vk_semaphore& acquire_semaphore();

		command_graph_node& create_node();
		command_graph_node& create_tail_node();

		void submit(
			VkQueue queue,
			VkFence dst_fence,
			size_t src_semaphores_count = 0,
			VkSemaphore* src_semaphores = nullptr,
			size_t dst_semaphores_count = 0,
			VkSemaphore* dst_semaphores = nullptr
		);
	};
}

