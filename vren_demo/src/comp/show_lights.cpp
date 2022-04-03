#include "show_lights.hpp"

vren_demo::show_lights::show_lights(vren::renderer& renderer) :
	m_renderer(&renderer),

	m_shader(std::make_shared<vren::vk_utils::self_described_shader>(
		vren::vk_utils::load_and_describe_shader(renderer.m_toolbox->m_context, "./resources/shaders/show_lights.comp.bin")
	)),
	m_pipeline(
		vren::vk_utils::create_compute_pipeline(renderer.m_toolbox->m_context, m_shader)
	)
{}

void vren_demo::show_lights::write_instance_buffer_descriptor_set(VkDescriptorSet desc_set, VkBuffer instance_buf)
{
	auto& ctx = m_renderer->m_toolbox->m_context;

	VkDescriptorBufferInfo buf_info{
		.buffer = instance_buf,
		.offset = 0,
		.range  = VK_WHOLE_SIZE,
	};

	VkWriteDescriptorSet desc_set_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = desc_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &buf_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(ctx->m_device, 1, &desc_set_write, 0, nullptr);
}

void vren_demo::show_lights::dispatch(
	int frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	vren::light_array const& light_arr,
	vren::render_object& point_light_visualizer,
	push_constants push_constants
)
{
	size_t point_lights_num = light_arr.m_point_lights.size();

	if (point_light_visualizer.m_instances_count < point_lights_num)
	{
		point_light_visualizer.set_instances_buffer(std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::create_instances_buffer(*m_renderer->m_toolbox, nullptr, point_lights_num)
		), point_lights_num);
	}

	point_light_visualizer.m_instances_count = point_lights_num;

	if (point_lights_num == 0) {
		return;
	}

	/* */

	m_pipeline.bind(cmd_buf);

	/* light_arr_desc_set */
	auto light_arr_desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		m_renderer->m_toolbox->m_descriptor_pool->acquire(m_shader->get_descriptor_set_layout(0))
	);
	m_renderer->write_light_array_descriptor_set(frame_idx, light_arr_desc_set->m_handle.m_descriptor_set);
	m_pipeline.bind_descriptor_set(cmd_buf, 0, light_arr_desc_set->m_handle.m_descriptor_set);
	res_container.add_resource(light_arr_desc_set);

	/* point_lights_inst_buf */
	auto point_lights_inst_buf_desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		m_renderer->m_toolbox->m_descriptor_pool->acquire(m_shader->get_descriptor_set_layout(1))
	);
	write_instance_buffer_descriptor_set(point_lights_inst_buf_desc_set->m_handle.m_descriptor_set, point_light_visualizer.m_instances_buffer->m_buffer.m_handle);
	m_pipeline.bind_descriptor_set(cmd_buf, 1, point_lights_inst_buf_desc_set->m_handle.m_descriptor_set);
	res_container.add_resource(point_lights_inst_buf_desc_set);

	/* */

	m_pipeline.push_constants(cmd_buf, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants));

	/* */

	m_pipeline.dispatch(cmd_buf, (uint32_t) glm::ceil(float(point_lights_num) / 512.0f), 1, 1);
}
