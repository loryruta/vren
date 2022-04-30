#include "render_graph.hpp"

#include <unordered_set>

#include "utils/log.hpp"

bool does_segments_intersect(float x1, float x2, float y1, float y2)
{
	// https://eli.thegreenplace.net/2008/08/15/intersection-of-1d-segments
	return x2 >= y1 && y2 >= x1;
}

// --------------------------------------------------------------------------------------------------------------------------------
// Buffer resource
// --------------------------------------------------------------------------------------------------------------------------------

bool vren::render_graph_node_buffer_resource::operator==(render_graph_node_buffer_resource const& other) const
{
	return m_buffer == other.m_buffer &&
		does_segments_intersect(
			m_offset, m_offset + m_size,
			other.m_offset, other.m_offset + other.m_size
		);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph node
// --------------------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph
// --------------------------------------------------------------------------------------------------------------------------------

void traverse_node(vren::render_graph_node const& node, bool callback_this_node, std::function<bool(vren::render_graph_node const& node)> const& callback)
{
	if (callback_this_node && !callback(node)) {
		return;
	}

	for (vren::render_graph_node const* following_node : node.get_following_nodes()) {
		traverse_node(*following_node, true, callback);
	}
}

void vren::render_graph_executor::initialize_image_layout(vren::render_graph_node const& node, std::tuple<vren::render_graph_node_image_resource, VkImageLayout, VkAccessFlags> const& image_resource, VkCommandBuffer command_buffer)
{
	auto const& [image, image_layout, access_flags] = image_resource;
	VkImageMemoryBarrier image_memory_barrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = NULL,
		.dstAccessMask = access_flags,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = image_layout,
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
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, node.m_in_stage, NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void vren::render_graph_executor::place_barriers(vren::render_graph_node const& node, VkCommandBuffer command_buffer)
{
	// If a resource of the current node is also required by one of the following nodes, then we have to
	// place a barrier soon after the current node. Once a node sharing the same resource is hit, we prune
	// the following branch containing the resource.

	for (auto const& resource_1 : node.m_image_resources)
	{
		std::unordered_set<vren::render_graph_node const*> generated_barriers; // Contains nodes for which a barrier has been generated and thus shouldn't be generated anymore

		traverse_node(node, /* callback_this_node */ false, [&](vren::render_graph_node const& following_node) -> bool
		{
			if (generated_barriers.contains(&following_node)) { // A barrier for this node has already been generated, we can prune the branch
				return false;
			}

			for (auto const& resource_2 : following_node.m_image_resources)
			{
				auto const& [image_1, image_layout_1, access_flags_1] = resource_1;
				auto const& [image_2, image_layout_2, access_flags_2] = resource_2;

				if (image_1 == image_2)
				{
					VkImageMemoryBarrier image_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = access_flags_1,
						.dstAccessMask = access_flags_2,
						.oldLayout = image_layout_1,
						.newLayout = image_layout_2,
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
					vkCmdPipelineBarrier(command_buffer, node.m_out_stage, following_node.m_in_stage, NULL, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

					//VREN_INFO("render_graph_executor | \"{}\" < Pipeline barrier for image \"{}\" > \"{}\"\n", node.m_name, image_1.m_name, following_node.m_name);

					generated_barriers.emplace(&following_node);

					// Since we've found a child node sharing the same resource, there's no need to proceed with this node's children
					// to find out further nodes sharing the same resource along this branch. Those nodes will be covered when the
					// current "following node" will be the subject.
					return false;
				}
			}

			return true;
		});
	}

	for (auto const& resource_1 : node.m_buffer_resources)
	{
		std::unordered_set<vren::render_graph_node const*> generated_barriers;

		traverse_node(node, /* callback_this_node */ false, [&](vren::render_graph_node const& following_node) -> bool
		{
			if (generated_barriers.contains(&following_node)) {
				return false;
			}

			for (auto const& resource_2 : following_node.m_buffer_resources)
			{
				auto const& [buffer_1, access_flags_1] = resource_1;
				auto const& [buffer_2, access_flags_2] = resource_2;

				if (buffer_1 == buffer_2)
				{
					VkBufferMemoryBarrier buffer_memory_barrier{
						.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
						.pNext = nullptr,
						.srcAccessMask = access_flags_1,
						.dstAccessMask = access_flags_2,
						.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
						.buffer = buffer_1.m_buffer,
						.offset = buffer_1.m_offset,
						.size = buffer_1.m_size
					};
					vkCmdPipelineBarrier(command_buffer, node.m_out_stage, following_node.m_in_stage, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

					//VREN_INFO("render_graph_executor | \"{}\" < Pipeline barrier for buffer \"{}\" > \"{}\"\n", node.m_name, buffer_1.m_name, following_node.m_name);

					return false;
				}
			}

			return true;
		});
	}
}

void vren::render_graph_executor::execute_node(vren::render_graph_node const& node, uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
{
	//VREN_INFO("render_graph_executor | Executing node: \"{}\"\n", node.m_name);

	node.m_callback(frame_idx, command_buffer, resource_container);
}

void vren::render_graph_executor::execute(vren::render_graph_t const& render_graph, uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
{
	//VREN_INFO("render_graph_executor | Execute\n");

	std::unordered_set<vren::render_graph_node*> concurrent_nodes; // The list of nodes that can be executed in parallel without interleaving barriers
	concurrent_nodes.insert(render_graph.begin(), render_graph.end());

	// Images for which an initial transition hasn't already been done
	std::unordered_set<vren::render_graph_node_image_resource, vren::render_graph_node_image_resource_hasher> initial_image_layout_transited;

	while (concurrent_nodes.size() > 0)
	{
		// Transit images that hasn't been transited to their correct layout yet
		for (vren::render_graph_node const* node : concurrent_nodes)
		{
			for (auto const& resource : node->m_image_resources)
			{
				auto const& [image, image_layout, access_flags] = resource;
				if (image_layout != VK_IMAGE_LAYOUT_UNDEFINED && !initial_image_layout_transited.contains(image))
				{
					initialize_image_layout(*node, resource, command_buffer);
					initial_image_layout_transited.insert(image);
				}
			}
		}

		// Now all concurrent nodes - roughly same depth level's nodes - can be executed concurrently
		for (vren::render_graph_node const* node : concurrent_nodes)
		{
			execute_node(*node, frame_idx, command_buffer, resource_container);
		}

		// We place barriers after these nodes for every resource (image or buffer) that has to be synchronized for future access
		for (vren::render_graph_node const* node : concurrent_nodes)
		{
			place_barriers(*node, command_buffer);
		}

		// We can go on with the next level by taking the next nodes of every current' level node
		std::unordered_set<vren::render_graph_node*> next_concurrent_nodes; // Unordered set is used to avoid duplicated nodes
		for (vren::render_graph_node const* node : concurrent_nodes)
		{
			auto const& following_nodes = node->get_following_nodes();
			next_concurrent_nodes.insert(following_nodes.begin(), following_nodes.end());
		}

		concurrent_nodes = std::move(next_concurrent_nodes);
	}

	//VREN_INFO("render_graph_executor | Done\n");
}

// --------------------------------------------------------------------------------------------------------------------------------
// Render-graph handler
// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph_handler::~render_graph_handler()
{
	// TODO deallocate the render_graph
}

// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph_node* vren::clear_color_buffer(VkImage color_buffer, VkClearColorValue clear_color_value)
{
	auto node = new vren::render_graph_node();
	node->set_name("clear_color_buffer");
	node->set_in_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_out_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image(
		{ .m_name = "color_buffer", .m_image = color_buffer, .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT },
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT
	);
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageSubresourceRange subresource_range =
			{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, };
		vkCmdClearColorImage(command_buffer, color_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color_value, 1, &subresource_range);
	});
	return node;
}

vren::render_graph_node* vren::clear_depth_stencil_buffer(VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value)
{
	auto node = new vren::render_graph_node();
	node->set_name("clear_depth_stencil_buffer");
	node->set_in_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_out_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image(
		{ .m_name = "depth_buffer", .m_image = depth_buffer, .m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT },
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT
	);
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageSubresourceRange subresource_range =
			{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1, };
		vkCmdClearDepthStencilImage(command_buffer, depth_buffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_depth_stencil_value, 1, &subresource_range);
	});
	return node;
}
