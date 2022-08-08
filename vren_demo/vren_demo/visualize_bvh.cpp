#include "visualize_bvh.hpp"

#include <vren/toolbox.hpp>
#include <vren/primitives/build_bvh.hpp>
#include <vren/vk_helpers/misc.hpp>

vren_demo::visualize_bvh::visualize_bvh(vren::context const& context) :
	m_context(&context),
    m_pipeline([&]()
	{
		vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/visualize_bvh.comp.spv");
		vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
		return vren::create_compute_pipeline(*m_context, shader);
	}())
{
}

vren::render_graph_t vren_demo::visualize_bvh::write(
	vren::render_graph_allocator& render_graph_allocator,
	vren::vk_utils::buffer const& bvh,
	uint32_t level_count,
	vren::debug_renderer_draw_buffer const& draw_buffer
)
{
	vren::render_graph_node* node = render_graph_allocator.allocate();
	node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->add_buffer({ .m_buffer = bvh.m_buffer.m_handle }, VK_ACCESS_SHADER_READ_BIT);
	node->add_buffer({ .m_buffer = draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle }, VK_ACCESS_SHADER_WRITE_BIT);
	node->set_callback([this, &bvh, level_count, &draw_buffer](
		uint32_t frame_idx,
		VkCommandBuffer command_buffer,
		vren::resource_container& resource_container
	)
	{
		m_pipeline.bind(command_buffer);

		uint32_t offset = 0;
		for (int32_t level = level_count - 1; level >= 0; level--)
		{
			uint32_t node_count = 1 << (5 * level);

			struct
			{
				uint32_t m_node_offset;
				uint32_t m_node_count;
				uint32_t m_color;
			} push_constants;

			uint32_t colors[]{ 0xff0000, 0xffff00, 0x00ff00, 0x0000ff, 0x00ffff, 0xff00ff, 0xffffff };

			push_constants = {
				.m_node_offset = offset,
				.m_node_count = node_count,
				.m_color = colors[level_count - level + 1],
			};
			m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

			auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
				m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
			);
			resource_container.add_resource(descriptor_set);

			vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 0, bvh.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
			vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set->m_handle.m_descriptor_set, 1, draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle, VK_WHOLE_SIZE, 0);

			m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

			uint32_t num_workgroups = vren::divide_and_ceil(node_count, 1024);
			m_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

			offset += node_count;
		}
	});
	return vren::render_graph_gather(node);
}
