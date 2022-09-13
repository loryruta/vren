#include "blit_depth_buffer_pyramid.hpp"

#include <vren/toolbox.hpp>

vren_demo::blit_depth_buffer_pyramid::blit_depth_buffer_pyramid(vren::context const& context) :
    m_context(&context),
	m_pipeline([&]()
	{
		vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/blit_depth_buffer_pyramid.comp.spv");
		vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
		return vren::create_compute_pipeline(*m_context, shader);
	}()),
	m_sampler(vren::vk_utils::create_sampler(
		*m_context,
		VK_FILTER_NEAREST,
		VK_FILTER_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_REPEAT
	))
{
}

vren::render_graph_t vren_demo::blit_depth_buffer_pyramid::operator()(
	vren::render_graph_allocator& allocator,
	vren::depth_buffer_pyramid const& depth_buffer_pyramid,
	uint32_t level,
	vren::vk_utils::combined_image_view const& color_buffer,
	uint32_t width,
	uint32_t height
)
{
	vren::render_graph_node* node = allocator.allocate();
	node->set_name("blit_depth_buffer_pyramid");
	node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->add_image({
		.m_name = "depth_buffer_pyramid",
		.m_image = depth_buffer_pyramid.get_image(),
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_mip_level = level,
	}, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT);
	node->add_image({
		.m_name = "color_buffer",
		.m_image = color_buffer.get_image(),
		.m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		.m_mip_level = 0,
	}, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);
	node->set_callback([
		=,
		&depth_buffer_pyramid,
		&color_buffer
	](
		uint32_t frame_idx,
		VkCommandBuffer command_buffer,
		vren::resource_container& resource_container
	)
	{
		m_pipeline.bind(command_buffer);

		auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
		);
		vren::vk_utils::write_combined_image_sampler_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, m_sampler.m_handle, depth_buffer_pyramid.get_level_image_view(level), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, color_buffer.get_image_view(), VK_IMAGE_LAYOUT_GENERAL);

		m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

		uint32_t num_workgroups_x = vren::divide_and_ceil(width, 32);
		uint32_t num_workgroups_y = vren::divide_and_ceil(height, 32);
		m_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);

		resource_container.add_resource(descriptor_set);
	});
	return vren::render_graph_gather(node);
}
