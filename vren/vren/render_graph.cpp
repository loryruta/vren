#include "render_graph.hpp"

#include <stdexcept>
#include <bitset>
#include <fstream>

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph
// --------------------------------------------------------------------------------------------------------------------------------

void vren::render_graph::traverse(allocator& allocator, graph_t const& graph, traverse_callback_t const& callback, bool forward_traversal, bool callback_initial_nodes)
{
	const size_t k_max_allocable_nodes = vren::render_graph::allocator::k_max_allocable_nodes;

	graph_t stepping_graph[2];
	bool _1 = false; // true if buffer 1 is the front buffer

	stepping_graph[_1] = graph;

	while (stepping_graph[_1].size() > 0)
	{
		std::bitset<k_max_allocable_nodes> already_taken{};

		stepping_graph[!_1].clear();

		for (node_idx_t node_idx : stepping_graph[_1])
		{
			if (!callback_initial_nodes || callback(node_idx))
			{
				auto const& next_nodes =
					forward_traversal ? allocator.get_node_at(node_idx)->get_next_nodes() : allocator.get_node_at(node_idx)->get_previous_nodes(); // Maybe constexpr condition would make it go faster?
				for (node_idx_t next_node_idx : next_nodes)
				{
					if (!already_taken[next_node_idx])
					{
						stepping_graph[!_1].push_back(next_node_idx);
						already_taken[next_node_idx] = true;
					}
				}
			}
		}

		if (!callback_initial_nodes)
		{
			callback_initial_nodes = true;
		}

		_1 = !_1;
	}
}

vren::render_graph::graph_t vren::render_graph::get_starting_nodes(allocator& allocator, graph_t const& graph)
{
	return graph;
}

vren::render_graph::graph_t vren::render_graph::get_ending_nodes(vren::render_graph::allocator& allocator, vren::render_graph::graph_t const& graph)
{
	graph_t result;
	vren::render_graph::traverse(allocator, graph, [&](node_idx_t node_idx)
	{
		if (allocator.get_node_at(node_idx)->get_next_nodes().empty()) {
			result.push_back(node_idx);
		}
		return true;
	}, true, true);
	return result;
}

vren::render_graph::graph_t vren::render_graph::concat(allocator& allocator, graph_t const& left, graph_t const& right)
{
	if (left.empty())
	{
		return right;
	}
	else if (right.empty())
	{
		return left;
	}
	else
	{
		// Merge the left graph with the right graph and return the tail of the merged result
		for (node_idx_t left_node_idx : get_ending_nodes(allocator, left))
		{
			for (node_idx_t right_node_idx : get_starting_nodes(allocator, right))
			{
				allocator.get_node_at(left_node_idx)->add_next(allocator.get_node_at(right_node_idx));
			}
		}

		return get_ending_nodes(allocator, left);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph execution logic
// --------------------------------------------------------------------------------------------------------------------------------

template<
	typename _execution_parameters,
	typename _make_initial_image_layout_transition,
	typename _execute_node,
	typename _put_image_memory_barrier,
	typename _put_buffer_memory_barrier
>
void vren::render_graph::detail::execute(
	vren::render_graph::allocator& allocator,
	vren::render_graph::graph_t const& graph,
	_execution_parameters& params
)
{
	const size_t k_max_allocable_nodes = vren::render_graph::allocator::k_max_allocable_nodes;

	_make_initial_image_layout_transition make_initial_image_layout_transition{};
	_execute_node execute_node{};
	_put_image_memory_barrier put_image_memory_barrier{};
	_put_buffer_memory_barrier put_buffer_memory_barrier{};

	graph_t stepping_graph[2];
	bool _1 = false;

	std::bitset<k_max_allocable_nodes> executed_nodes{};

	stepping_graph[_1] = graph;

	while (stepping_graph[_1].size() > 0)
	{
		std::bitset<k_max_allocable_nodes> already_taken{};

		stepping_graph[!_1].clear();

		for (node_idx_t node_idx : stepping_graph[_1])
		{
			auto node = allocator.get_node_at(node_idx);

			// The node can be executed only if its previous nodes have been executed as well
			bool can_execute = true;
			for (vren::render_graph::node_idx_t previous_node_idx : node->get_previous_nodes())
			{
				if (!executed_nodes[previous_node_idx])
				{
					can_execute = false;
					break;
				}
			}

			if (!can_execute)
			{
				// If the node can't be executed, place it in the next execution pool
				if (!already_taken[node_idx])
				{
					stepping_graph[!_1].push_back(node_idx);
					already_taken[node_idx] = true;
				}
				continue;
			}

			// We need to know if an image required by the current node has already been transited or not: in order to acknowledge it,
			// we traverse the render-graph backward starting from the current node and see if any other previous node shares a conflicting image. If this happens it means
			// the current one has already been transited
			for (uint32_t image_idx = 0; image_idx < node->get_image_accesses().size(); image_idx++)
			{
				bool need_init = true;

				vren::render_graph::traverse(allocator, {node_idx}, [&](node_idx_t node_2_idx) -> bool
				{
					auto node_2 = allocator.get_node_at(node_2_idx);
					for (uint32_t image_2_idx = 0; image_2_idx < node_2->get_image_accesses().size(); image_2_idx++)
					{
						auto& image_1 = node->get_image_accesses().at(image_idx);
						auto& image_2 = node_2->get_image_accesses().at(image_2_idx);
						if (image_1.does_conflict(image_2))
						{
							need_init = false;
							return false;
						}
					}
					return true;
				}, /* Forward traversal */ false, /* Initial callback */ false);

				if (need_init) {
					make_initial_image_layout_transition(*node, image_idx, params);
				}
			}

			// Execute the node and mark it as executed
			execute_node(*node, params);
			executed_nodes[node_idx] = true;

			// If an image resource of the current node will be used by a following node, then place a barrier soon after it
			for (uint32_t image_idx = 0; image_idx < node->get_image_accesses().size(); image_idx++)
			{
				vren::render_graph::traverse(allocator, {node_idx}, [&](node_idx_t node_2_idx) -> bool
				{
					auto node_2 = allocator.get_node_at(node_2_idx);
					for (uint32_t image_2_idx = 0; image_2_idx < node_2->get_image_accesses().size(); image_2_idx++)
					{
						auto& image_1 = node->get_image_accesses().at(image_idx);
						auto& image_2 = node_2->get_image_accesses().at(image_2_idx);
						if (image_1.does_conflict(image_2))
						{
							put_image_memory_barrier(*node, *node_2, image_idx, image_2_idx, params);
							return false;
						}
					}

					return true;
				}, /* Forward traversal */ true, /* Initial callback */ false);
			}

			// The same as above for buffer resources
			for (uint32_t buffer_idx = 0; buffer_idx < node->get_buffer_accesses().size(); buffer_idx++)
			{
				vren::render_graph::traverse(allocator, {node_idx}, [&](node_idx_t node_2_idx) -> bool
				{
					auto node_2 = allocator.get_node_at(node_2_idx);
					for (uint32_t buffer_2_idx = 0; buffer_2_idx < node_2->get_buffer_accesses().size(); buffer_2_idx++)
					{
						auto& buffer_1 = node->get_buffer_accesses().at(buffer_idx);
						auto& buffer_2 = node_2->get_buffer_accesses().at(buffer_2_idx);
						if (buffer_1.does_conflict(buffer_2))
						{
							put_buffer_memory_barrier(*node, *node_2, buffer_idx, buffer_2_idx, params);
							return false;
						}
					}

					return true;
				}, /* Forward traversal */ true, /* Initial callback */ false);
			}

			// Put current node's children to the execution pool and repeat
			for (vren::render_graph::node_idx_t next_node_idx : node->get_next_nodes())
			{
				if (!already_taken[next_node_idx])
				{
					stepping_graph[!_1].push_back(next_node_idx);
					already_taken[next_node_idx] = true;
				}
			}
		}

		_1 = !_1;
	}
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph execution
// --------------------------------------------------------------------------------------------------------------------------------

void vren::render_graph::execute(
	vren::render_graph::allocator& allocator,
	vren::render_graph::graph_t const& graph,
	uint32_t frame_idx,
	VkCommandBuffer command_buffer,
	vren::resource_container& resource_container
)
{
	struct execution_parameters
	{
		uint32_t m_frame_idx;
		VkCommandBuffer m_command_buffer;
		vren::resource_container& m_resource_container;
	};

	struct make_initial_image_layout_transition
	{
		void operator()(node const& node, uint32_t image_idx, execution_parameters const& params) const
		{
			auto image_access = node.get_image_accesses().at(image_idx);
			VkImageMemoryBarrier image_memory_barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = NULL,
				.dstAccessMask = image_access.m_access_flags,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = image_access.m_image_layout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image_access.m_image,
				.subresourceRange = {
					.aspectMask = image_access.m_image_aspect,
					.baseMipLevel = image_access.m_mip_level,
					.levelCount = 1,
					.baseArrayLayer = image_access.m_layer,
					.layerCount = 1
				}
			};
			vkCmdPipelineBarrier(params.m_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, node.get_src_stage(), NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
		}
	};

	struct execute_node
	{
		void operator()(node const& node, execution_parameters const& params)
		{
			node(params.m_frame_idx, params.m_command_buffer, params.m_resource_container);
		}
	};

	struct put_image_memory_barrier
	{
		void operator()(node const& node_1, node const& node_2, uint32_t image_1_idx, uint32_t image_2_idx, execution_parameters const& params) const
		{
			auto image_1 = node_1.get_image_accesses().at(image_1_idx);
			auto image_2 = node_2.get_image_accesses().at(image_2_idx);

			VkImageMemoryBarrier image_memory_barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = image_1.m_access_flags,
				.dstAccessMask = image_2.m_access_flags,
				.oldLayout = image_1.m_image_layout,
				.newLayout = image_2.m_image_layout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image_1.m_image,
				.subresourceRange = {
					.aspectMask = image_1.m_image_aspect,
					.baseMipLevel = image_1.m_mip_level,
					.levelCount = 1,
					.baseArrayLayer = image_1.m_layer,
					.layerCount = 1,
				}
			};
			vkCmdPipelineBarrier(params.m_command_buffer, node_1.get_dst_stage(), node_2.get_src_stage(), NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
		}
	};

	struct put_buffer_memory_barrier
	{
		void operator()(node const& node_1, node const& node_2, uint32_t buffer_1_idx, uint32_t buffer_2_idx, execution_parameters const& params) const
		{
			auto& buffer_1 = node_1.get_buffer_accesses().at(buffer_1_idx);
			auto& buffer_2 = node_2.get_buffer_accesses().at(buffer_2_idx);

			VkBufferMemoryBarrier buffer_memory_barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = buffer_1.m_access_flags,
				.dstAccessMask = buffer_2.m_access_flags,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = buffer_1.m_buffer,
				.offset = 0,//buffer_access_2.m_offset,
				.size = VK_WHOLE_SIZE//buffer_access_1.m_size
			};
			vkCmdPipelineBarrier(params.m_command_buffer, node_1.get_dst_stage(), node_2.get_src_stage(), NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
		}
	};

	execution_parameters params{ .m_frame_idx = frame_idx, .m_command_buffer = command_buffer, .m_resource_container = resource_container };
	vren::render_graph::detail::execute<
		execution_parameters const,
		make_initial_image_layout_transition,
		execute_node,
		put_image_memory_barrier,
		put_buffer_memory_barrier
	>(allocator, graph, params);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph dump
// --------------------------------------------------------------------------------------------------------------------------------

void vren::render_graph::dump(
	vren::render_graph::allocator& allocator,
	vren::render_graph::graph_t const& graph,
	std::ostream& output
)
{
	struct execution_parameters
	{
		std::ostream& m_writer;
		uint32_t m_position = 0;
	};

	struct make_initial_image_layout_transition
	{
		void operator()(node const& node, uint32_t image_idx, execution_parameters& params) const
		{
			auto const& image = node.get_image_accesses().at(image_idx);
			params.m_writer << fmt::format("node_{}_image_{} [color=red, fillcolor=orange, label=\"{} ({})\"];\n", node.get_idx(), image_idx, image.m_name, params.m_position);
			params.m_position++;
		}
	};

	struct execute_node
	{
		void operator()(node const& node, execution_parameters& params) const
		{
			params.m_writer << fmt::format("node_{} [shape=rect, label=\"{} ({})\"];\n", node.get_idx(), node.get_name(), params.m_position);
			params.m_position++;
		}
	};

	struct put_image_memory_barrier
	{
		void operator()(node const& node_1, node const& node_2, uint32_t image_1_idx, uint32_t image_2_idx, execution_parameters& params) const
		{
			params.m_writer << fmt::format("node_{}_image_{} -> node_{}_image_{} [color=red, xlabel=\"({})\"]", node_1.get_idx(), image_1_idx, node_2.get_idx(), image_2_idx, params.m_position);
			params.m_position++;
		}
	};

	struct put_buffer_memory_barrier
	{
		void operator()(node const& node_1, node const& node_2, uint32_t buffer_1_idx, uint32_t buffer_2_idx, execution_parameters& params) const
		{
			params.m_writer << fmt::format("node_{}_image_{} -> node_{}_image_{} [color=red, xlabel=\"({})\"]", node_1.get_idx(), buffer_1_idx, node_2.get_idx(), buffer_2_idx, params.m_position);
			params.m_position++;
		}
	};

	output << "digraph render_graph {\n";

	output << "node[shape=rect]\n";

	traverse(allocator, graph, [&](node_idx_t node_idx)
	{
		auto node = allocator.get_node_at(node_idx);

		output << fmt::format("node_{} [shape=rect, label=\"{}\"];\n", node_idx, node->get_name());

		for (uint32_t image_idx = 0; image_idx < node->get_image_accesses().size(); image_idx++)
		{
			auto image = node->get_image_accesses().at(image_idx);

			output << fmt::format("node_{}_image_{} [shape=rect, label=\"{}\", style=filled, fillcolor=yellow, color=gray, height=0];\n", node_idx, image_idx, image.m_name);
			output << fmt::format("node_{} -> node_{}_image_{} [arrowhead=none, color=gray];\n", node_idx, node_idx, image_idx);
		}

		for (uint32_t buffer_idx = 0; buffer_idx < node->get_buffer_accesses().size(); buffer_idx++)
		{
			auto buffer = node->get_buffer_accesses().at(buffer_idx);

			output << fmt::format("node_{}_buffer_{} [shape=rect, label=\"{}\", style=filled, fillcolor=yellow, color=gray, height=0];\n", node_idx, buffer_idx, buffer.m_name);
			output << fmt::format("node_{} -> node_{}_buffer_{} [arrowhead=none, color=gray];\n", node_idx, node_idx, buffer_idx);
		}

		for (node_idx_t next_node_idx : node->get_next_nodes())
		{
			output << fmt::format("node_{} -> node_{};\n", node_idx, next_node_idx);
		}

		return true;
	}, /* Forward traversal */ true, /* Initial callback */ true);

	execution_parameters params{ .m_writer = output, .m_position = 0 };
	vren::render_graph::detail::execute<
		execution_parameters,
		make_initial_image_layout_transition,
		execute_node,
		put_image_memory_barrier,
		put_buffer_memory_barrier
	>(allocator, graph, params);

	output << "}";
}

// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph::graph_t vren::clear_color_buffer(render_graph::allocator& allocator, VkImage color_buffer, VkClearColorValue clear_color_value)
{
	auto node = allocator.allocate();
	node->set_name("clear_color_buffer");
	node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image({
		.m_name = "color_buffer",
		.m_image = color_buffer,
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.m_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT,
	});
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageSubresourceRange subresource_range =
			{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, };
		vkCmdClearColorImage(command_buffer, color_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &subresource_range);
	});
	return render_graph::gather(node);
}

vren::render_graph::graph_t vren::clear_depth_stencil_buffer(render_graph::allocator& allocator, VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value)
{
	auto node = allocator.allocate();
	node->set_name("clear_depth_stencil_buffer");
	node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image({
		.m_name = "depth_buffer",
		.m_image = depth_buffer,
		.m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
		.m_image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.m_access_flags = VK_ACCESS_TRANSFER_WRITE_BIT,
	});
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageSubresourceRange subresource_range =
			{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, };
		vkCmdClearDepthStencilImage(command_buffer, depth_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &subresource_range);
	});
	return render_graph::gather(node);
}
