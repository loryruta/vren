#include "render_graph.hpp"

#include <stdexcept>
#include <bitset>
#include <fstream>

#include <fmt/format.h>

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph
// --------------------------------------------------------------------------------------------------------------------------------

void vren::render_graph_traverse(
	vren::render_graph_allocator& allocator,
	vren::render_graph_t const& graph,
	render_graph_traverse_callback_t const& callback,
	bool forward_traversal,
	bool callback_initial_nodes
)
{
	const size_t k_max_allocable_nodes = vren::render_graph_allocator::k_max_allocable_nodes;

	vren::render_graph_t stepping_graph[2];
	bool _1 = false; // true if buffer 1 is the front buffer

	stepping_graph[_1] = graph;

	while (stepping_graph[_1].size() > 0)
	{
		std::bitset<k_max_allocable_nodes> already_taken{};

		stepping_graph[!_1].clear();

		for (vren::render_graph_node_index_t node_idx : stepping_graph[_1])
		{
			if (!callback_initial_nodes || callback(node_idx))
			{
				auto const& next_nodes =
					forward_traversal ? allocator.get_node_at(node_idx)->get_next_nodes() : allocator.get_node_at(node_idx)->get_previous_nodes(); // Maybe constexpr condition would make it go faster?
				
				for (vren::render_graph_node_index_t next_node_idx : next_nodes)
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

vren::render_graph_t vren::render_graph_get_start(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph)
{
	return graph;
}

vren::render_graph_t vren::render_graph_get_end(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph)
{
	vren::render_graph_t result;

	vren::render_graph_traverse(allocator, graph, [&](vren::render_graph_node_index_t node_idx)
	{
		if (allocator.get_node_at(node_idx)->get_next_nodes().empty()) {
			result.push_back(node_idx);
		}
		return true;
	}, true, true);

	return result;
}

vren::render_graph_t vren::render_graph_concat(vren::render_graph_allocator& allocator, vren::render_graph_t const& left, vren::render_graph_t const& right)
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
		for (vren::render_graph_node_index_t left_node_idx : vren::render_graph_get_end(allocator, left))
		{
			for (vren::render_graph_node_index_t right_node_idx : vren::render_graph_get_start(allocator, right))
			{
				allocator.get_node_at(left_node_idx)->add_next(allocator.get_node_at(right_node_idx));
			}
		}

		return vren::render_graph_get_end(allocator, left);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph abstract executor
// --------------------------------------------------------------------------------------------------------------------------------

void vren::detail::render_graph_executor::execute(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph)
{
	const size_t k_max_allocable_nodes = vren::render_graph_allocator::k_max_allocable_nodes;
	const size_t k_max_image_infos = vren::render_graph_allocator::k_max_image_infos;

	vren::render_graph_t stepping_graph[2];
	bool _1 = false;

	std::bitset<k_max_allocable_nodes> executed_nodes{};
	std::bitset<k_max_image_infos> image_transited{};

	stepping_graph[_1] = graph;

	while (stepping_graph[_1].size() > 0)
	{
		std::bitset<k_max_allocable_nodes> already_taken{};

		stepping_graph[!_1].clear();

		for (vren::render_graph_node_index_t node_idx : stepping_graph[_1])
		{
			vren::render_graph_node* node = allocator.get_node_at(node_idx);

			// The node can be executed only if its previous nodes have been executed as well
			bool can_execute = true;
			for (vren::render_graph_node_index_t previous_node_idx : node->get_previous_nodes())
			{
				if (!executed_nodes[previous_node_idx])
				{
					can_execute = false;
					break;
				}
			}

			// If the node can't be executed, place it in the next execution pool
			if (!can_execute)
			{
				if (!already_taken[node_idx])
				{
					stepping_graph[!_1].push_back(node_idx);
					already_taken[node_idx] = true;
				}
				continue;
			}

			// If an image for this node hasn't already been transited we need to record initial layout transition
			for (vren::render_graph_node_image_access const& image_access : node->get_image_accesses())
			{
				if (!image_transited.test(image_access.m_image_idx))
				{
					make_initial_image_layout_transition(*node, image_access);
					image_transited[image_access.m_image_idx] = true;
				}
			}

			// Execute the node and mark it as executed
			execute_node(*node);
			executed_nodes[node_idx] = true;

			// Place image barriers
			for (auto const& image_access : node->get_image_accesses())
			{
				vren::render_graph_traverse(allocator, { node_idx }, [&](vren::render_graph_node_index_t node_idx_2) -> bool
				{
					vren::render_graph_node* node_2 = allocator.get_node_at(node_idx_2);

					for (vren::render_graph_node_image_access const& image_access_2 : node_2->get_image_accesses())
					{
						if (image_access.m_image_idx == image_access_2.m_image_idx)
						{
							place_image_memory_barrier(*node, *node_2, image_access, image_access_2);
							return false; // If the barrier has been placed we don't descend this node's children
						}
					}
					return true;

				}, true, false);
			}

			// Place buffer barriers
			for (auto const& buffer_access : node->get_buffer_accesses())
			{
				vren::render_graph_traverse(allocator, { node_idx }, [&](vren::render_graph_node_index_t node_idx_2) -> bool
				{
					vren::render_graph_node* node_2 = allocator.get_node_at(node_idx_2);

					for (vren::render_graph_node_buffer_access const& buffer_access_2 : node_2->get_buffer_accesses())
					{
						if (buffer_access.m_buffer_idx == buffer_access_2.m_buffer_idx)
						{
							place_buffer_memory_barrier(*node, *node_2, buffer_access, buffer_access_2);
							return false;
						}
					}

				}, true, false);
			}

			// Put current node's following nodes to the next execution pool and repeat
			for (vren::render_graph_node_index_t next_node_idx : node->get_next_nodes())
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
// Render-graph executor
// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph_executor::render_graph_executor(uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container) :
	m_frame_idx(frame_idx),
	m_command_buffer(command_buffer),
	m_resource_container(&resource_container)
{
}

void vren::render_graph_executor::make_initial_image_layout_transition(vren::render_graph_node const& node, vren::render_graph_node_image_access const& image_access)
{
	vren::render_graph_image_info const& image = node.get_allocator()->get_image_info_at(image_access.m_image_idx);

	VkImageMemoryBarrier image_memory_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = NULL,
		.dstAccessMask = image_access.m_access_flags,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = image_access.m_image_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image.m_image,
		.subresourceRange = {
			.aspectMask = image.m_image_aspect,
			.baseMipLevel = image.m_mip_level,
			.levelCount = 1,
			.baseArrayLayer = image.m_layer,
			.layerCount = 1
		}
	};
	vkCmdPipelineBarrier(m_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, node.get_src_stage(), NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void vren::render_graph_executor::execute_node(vren::render_graph_node const& node)
{
	node(m_frame_idx, m_command_buffer, *m_resource_container);
}

void vren::render_graph_executor::place_image_memory_barrier(
	vren::render_graph_node const& node_1,
	vren::render_graph_node const& node_2,
	vren::render_graph_node_image_access const& image_access_1,
	vren::render_graph_node_image_access const& image_access_2
)
{
	vren::render_graph_image_info const& image_1 = node_1.get_allocator()->get_image_info_at(image_access_1.m_image_idx);
	vren::render_graph_image_info const& image_2 = node_2.get_allocator()->get_image_info_at(image_access_2.m_image_idx);

	VkImageMemoryBarrier image_memory_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = image_access_1.m_access_flags,
		.dstAccessMask = image_access_2.m_access_flags,
		.oldLayout = image_access_1.m_image_layout,
		.newLayout = image_access_2.m_image_layout,
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
	vkCmdPipelineBarrier(m_command_buffer, node_1.get_dst_stage(), node_2.get_src_stage(), NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void vren::render_graph_executor::place_buffer_memory_barrier(
	vren::render_graph_node const& node_1,
	vren::render_graph_node const& node_2,
	vren::render_graph_node_buffer_access const& buffer_access_1,
	vren::render_graph_node_buffer_access const& buffer_access_2
)
{
	vren::render_graph_buffer_info const& buffer_1 = node_1.get_allocator()->get_buffer_info_at(buffer_access_1.m_buffer_idx);
	vren::render_graph_buffer_info const& buffer_2 = node_2.get_allocator()->get_buffer_info_at(buffer_access_2.m_buffer_idx);

	VkBufferMemoryBarrier buffer_memory_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = buffer_access_1.m_access_flags,
		.dstAccessMask = buffer_access_2.m_access_flags,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.buffer = buffer_1.m_buffer,
		.offset = 0,//buffer_access_2.m_offset,
		.size = VK_WHOLE_SIZE//buffer_access_1.m_size
	};
	vkCmdPipelineBarrier(m_command_buffer, node_1.get_dst_stage(), node_2.get_src_stage(), NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
}

void vren::render_graph_executor::execute(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph)
{
	vren::detail::render_graph_executor::execute(allocator, graph);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph dumper
// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph_dumper::render_graph_dumper(std::ostream& output) :
	m_output(&output),
	m_execution_order(0)
{
}

void vren::render_graph_dumper::make_initial_image_layout_transition(vren::render_graph_node const& node, vren::render_graph_node_image_access const& image_access)
{
	vren::render_graph_image_info const& image_info = node.get_allocator()->get_image_info_at(image_access.m_image_idx);
	*m_output << fmt::format("node_{}_image_{} [color=red, fillcolor=orange, label=\"{} ({})\"];\n", node.get_idx(), image_access.m_image_idx, image_info.m_name, m_execution_order);
	m_execution_order++;
}

void vren::render_graph_dumper::execute_node(vren::render_graph_node const& node)
{
	*m_output << fmt::format("node_{} [shape=rect, label=\"{} ({})\"];\n", node.get_idx(), node.get_name(), m_execution_order);
	m_execution_order++;
}

void vren::render_graph_dumper::place_image_memory_barrier(
	vren::render_graph_node const& node_1,
	vren::render_graph_node const& node_2,
	vren::render_graph_node_image_access const& image_access_1,
	vren::render_graph_node_image_access const& image_access_2
)
{
	*m_output << fmt::format("node_{}_image_{} -> node_{}_image_{} [color=red, xlabel=\"({})\"]", node_1.get_idx(), image_access_1.m_image_idx, node_2.get_idx(), image_access_2.m_image_idx, m_execution_order);
	m_execution_order++;
}

void vren::render_graph_dumper::place_buffer_memory_barrier(
	vren::render_graph_node const& node_1,
	vren::render_graph_node const& node_2,
	vren::render_graph_node_buffer_access const& buffer_access_1,
	vren::render_graph_node_buffer_access const& buffer_access_2
)
{
	*m_output << fmt::format("node_{}_image_{} -> node_{}_image_{} [color=red, xlabel=\"({})\"]", node_1.get_idx(), buffer_access_1.m_buffer_idx, node_2.get_idx(), buffer_access_2.m_buffer_idx, m_execution_order);
	m_execution_order++;
}

void vren::render_graph_dumper::dump(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph)
{
	*m_output << "digraph render_graph {\n";
	*m_output << "node[shape=rect]\n";

	vren::render_graph_traverse(allocator, graph, [&](vren::render_graph_node_index_t node_idx)
	{
		auto node = allocator.get_node_at(node_idx);

		*m_output << fmt::format("node_{} [shape=rect, label=\"{}\"];\n", node_idx, node->get_name());

		for (vren::render_graph_node_image_access const& image_access : node->get_image_accesses())
		{
			vren::render_graph_image_info const& image_info = node->get_allocator()->get_image_info_at(image_access.m_image_idx);

			*m_output << fmt::format("node_{}_image_{} [shape=rect, label=\"{}\", style=filled, fillcolor=yellow, color=gray, height=0];\n", node_idx, image_access.m_image_idx, image_info.m_name);
			*m_output << fmt::format("node_{} -> node_{}_image_{} [arrowhead=none, color=gray];\n", node_idx, node_idx, image_access.m_image_idx);
		}

		for (vren::render_graph_node_buffer_access const& buffer_access : node->get_buffer_accesses())
		{
			vren::render_graph_buffer_info const& buffer_info = node->get_allocator()->get_buffer_info_at(buffer_access.m_buffer_idx);

			*m_output << fmt::format("node_{}_buffer_{} [shape=rect, label=\"{}\", style=filled, fillcolor=yellow, color=gray, height=0];\n", node_idx, buffer_access.m_buffer_idx, buffer_info.m_name);
			*m_output << fmt::format("node_{} -> node_{}_buffer_{} [arrowhead=none, color=gray];\n", node_idx, node_idx, buffer_access.m_buffer_idx);
		}

		for (vren::render_graph_node_index_t next_node_idx : node->get_next_nodes())
		{
			*m_output << fmt::format("node_{} -> node_{};\n", node_idx, next_node_idx);
		}

		return true;
	}, /* Forward traversal */ true, /* Initial callback */ true);

	vren::detail::render_graph_executor::execute(allocator, graph);

	*m_output << "}\n";
}

// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph_t vren::clear_color_buffer(vren::render_graph_allocator& allocator, VkImage color_buffer, VkClearColorValue clear_color_value)
{
	auto node = allocator.allocate();
	node->set_name("clear_color_buffer");
	node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image({
		.m_name = "color_buffer",
		.m_image = color_buffer,
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
	}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageSubresourceRange subresource_range =
			{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, };
		vkCmdClearColorImage(command_buffer, color_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &subresource_range);
	});
	return vren::render_graph_gather(node);
}

vren::render_graph_t vren::clear_depth_stencil_buffer(vren::render_graph_allocator& allocator, VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value)
{
	auto node = allocator.allocate();
	node->set_name("clear_depth_stencil_buffer");
	node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image({
		.m_name = "depth_buffer",
		.m_image = depth_buffer,
		.m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
	}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT);
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageSubresourceRange subresource_range =
			{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, };
		vkCmdClearDepthStencilImage(command_buffer, depth_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &subresource_range);
	});
	return vren::render_graph_gather(node);
}
