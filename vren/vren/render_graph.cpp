#include "render_graph.hpp"

#include <stdexcept>
#include <array>
#include <bitset>

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph
// --------------------------------------------------------------------------------------------------------------------------------

void vren::render_graph::traverse(allocator& allocator, graph_t const& graph, bool first_callback, traverse_callback_t const& callback)
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
			if (!first_callback || callback(node_idx))
			{
				for (node_idx_t next_node_idx : allocator.get_node_at(node_idx)->get_next_nodes())
				{
					if (!already_taken[next_node_idx])
					{
						stepping_graph[!_1].push_back(next_node_idx);
						already_taken[next_node_idx] = true;
					}
				}
			}
		}

		if (!first_callback)
		{
			first_callback = true;
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
	vren::render_graph::traverse(allocator, graph, true, [&](node_idx_t node_idx)
	{
		if (allocator.get_node_at(node_idx)->get_next_nodes().empty()) {
			result.push_back(node_idx);
		}
		return true;
	});
	return result;
}

vren::render_graph::graph_t vren::render_graph::concat(allocator& allocator, graph_t const& left, graph_t const& right)
{
	auto left_ending_nodes = get_ending_nodes(allocator, left);
	auto right_starting_nodes = get_starting_nodes(allocator, right);

	for (node_idx_t left_node_idx : left_ending_nodes)
	{
		for (node_idx_t right_node_idx : right_starting_nodes)
		{
			allocator.get_node_at(left_node_idx)->add_next(allocator.get_node_at(right_node_idx));
		}
	}

	return get_ending_nodes(allocator, left);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph executor
// --------------------------------------------------------------------------------------------------------------------------------

void vren::render_graph::execute(vren::render_graph::allocator& allocator, vren::render_graph::graph_t const& graph, uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
{
	const size_t k_max_allocable_nodes = vren::render_graph::allocator::k_max_allocable_nodes;
	const size_t k_max_image_accesses_per_node = vren::render_graph::node::k_max_image_accesses;

	graph_t stepping_graph[2];
	bool _1 = false;

	std::bitset<k_max_allocable_nodes> executed_nodes{};
	std::bitset<k_max_allocable_nodes * k_max_image_accesses_per_node> initial_image_layout_transited{};

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

			auto image_accesses = node->get_image_accesses();

			// If an image needed by this node hasn't already been transited to the correct layout, we do it now
			for (vren::render_graph::node::image_access_idx_t image_access_idx = 0; image_access_idx < image_accesses.size(); image_access_idx++)
			{
				if (!initial_image_layout_transited[image_access_idx])
				{
					auto image_access = image_accesses[image_access_idx];
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
					vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, node->get_src_stage(), NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
					initial_image_layout_transited[image_access_idx] = true;
				}
			}

			// Execute the node and mark it as executed
			(*node)(frame_idx, command_buffer, resource_container);
			executed_nodes[node_idx] = true;

			// If an image resource of the current node will be used by a following node, then place a barrier soon after it
			for (auto const& image_access_1 : node->get_image_accesses())
			{
				vren::render_graph::traverse(allocator, {node_idx}, false, [&](node_idx_t other_node_idx) -> bool
				{
					auto other_node = allocator.get_node_at(other_node_idx);
					for (auto const& image_access_2 : other_node->get_image_accesses())
					{
						if (image_access_1.does_conflict(image_access_2))
						{
							VkImageMemoryBarrier image_memory_barrier{
								.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
								.pNext = nullptr,
								.srcAccessMask = image_access_1.m_access_flags,
								.dstAccessMask = image_access_2.m_access_flags,
								.oldLayout = image_access_1.m_image_layout,
								.newLayout = image_access_2.m_image_layout,
								.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								.image = image_access_1.m_image,
								.subresourceRange = {
									.aspectMask = image_access_1.m_image_aspect,
									.baseMipLevel = image_access_1.m_mip_level,
									.levelCount = 1,
									.baseArrayLayer = image_access_1.m_layer,
									.layerCount = 1,
								}
							};
							vkCmdPipelineBarrier(command_buffer, node->get_dst_stage(), node->get_src_stage(), NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
							return false;
						}
					}

					return true;
				});
			}

			// The same as above for buffer resources
			for (auto const& buffer_access_1 : node->get_buffer_accesses())
			{
				vren::render_graph::traverse(allocator, {node_idx}, false, [&](node_idx_t other_node_idx) -> bool
				{
					auto other_node = allocator.get_node_at(other_node_idx);
					for (auto const& buffer_access_2 : other_node->get_buffer_accesses())
					{
						if (buffer_access_1.does_conflict(buffer_access_2))
						{
							VkBufferMemoryBarrier buffer_memory_barrier{
								.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
								.pNext = nullptr,
								.srcAccessMask = buffer_access_1.m_access_flags,
								.dstAccessMask = buffer_access_2.m_access_flags,
								.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
								.buffer = buffer_access_1.m_buffer,
								.offset = 0,//buffer_access_2.m_offset,
								.size = VK_WHOLE_SIZE//buffer_access_1.m_size
							};
							vkCmdPipelineBarrier(command_buffer, node->get_dst_stage(), other_node->get_src_stage(), NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
							return false;
						}
					}

					return true;
				});
			}

			// Put current node's children to the execution pool and repeat
			for (vren::render_graph::node_idx_t next_node_idx : node->get_next_nodes())
			{
				if (!already_taken[next_node_idx])
				{
					stepping_graph[!_1].push_back(node_idx);
					already_taken[next_node_idx] = true;
				}
			}
		}

		_1 = !_1;
	}
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
