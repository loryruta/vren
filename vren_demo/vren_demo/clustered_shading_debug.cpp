#include "clustered_shading_debug.hpp"

#include "toolbox.hpp"

vren_demo::visualize_clusters::visualize_clusters(
    vren::context const& context
) :
    m_context(&context),
	m_pipeline([&]()
	{
		vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/visualize_clusters.comp.spv");
		vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
		return vren::create_compute_pipeline(*m_context, shader);
	}())
{
}

vren::render_graph_t vren_demo::visualize_clusters::operator()(
	vren::render_graph_allocator& render_graph_allocator,
	glm::uvec2 const& screen,
	vren::vk_utils::combined_image_view const& cluster_reference_buffer,
	vren::vk_utils::buffer const& cluster_key_buffer,
	vren::vk_utils::combined_image_view const& output
)
{
	vren::render_graph_node* node = render_graph_allocator.allocate();

	node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	// cluster_reference_buffer isn't part of the render_graph (it's an internal buffer), we put a generic barrier before using it
	node->add_image({ .m_image = output.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = 0, .m_layer = 0, }, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);

	node->set_callback([
		this,
		screen,
		&cluster_reference_buffer,
		&cluster_key_buffer,
		&output
	](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkBufferMemoryBarrier buffer_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.buffer = cluster_key_buffer.m_buffer.m_handle,
			.offset = 0,
			.size = VK_WHOLE_SIZE
		};
		VkImageMemoryBarrier image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
			.newLayout = VK_IMAGE_LAYOUT_GENERAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = cluster_reference_buffer.get_image(),
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			}
		};
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 1, &image_memory_barrier);

		m_pipeline.bind(command_buffer);

		auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
		);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 0, cluster_reference_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 1, cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 2, output.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);

		m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);

		uint32_t num_workgroups_x = vren::divide_and_ceil(screen.x, 32);
		uint32_t num_workgroups_y = vren::divide_and_ceil(screen.y, 32);
		m_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);

		resource_container.add_resources(
			descriptor_set_0
		);
	});

	return vren::render_graph_gather(node);
}
