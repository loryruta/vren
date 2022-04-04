#include "show_lights.hpp"

vren_demo::show_lights::show_lights(vren::context const& ctx, vren::renderer& renderer) :
	m_renderer(&renderer),

	m_pipeline(vren::vk_utils::create_compute_pipeline(
		ctx,
		vren::vk_utils::load_shader_from_file(ctx, "./resources/shaders/show_lights.comp.bin")
	))
{}

void vren_demo::show_lights::write_instance_buffer_descriptor_set(VkDescriptorSet desc_set, VkBuffer instance_buf)
{
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
	vkUpdateDescriptorSets(m_context->m_device, 1, &desc_set_write, 0, nullptr);
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
			vren::vk_utils::create_instances_buffer(*m_context, nullptr, point_lights_num)
		), point_lights_num);
	}

	point_light_visualizer.m_instances_count = point_lights_num;

	if (point_lights_num == 0) {
		return;
	}

	/* */

	m_pipeline.bind(cmd_buf);

	/* Light array */
	m_pipeline.acquire_and_bind_descriptor_set(
		*m_context,
		cmd_buf,
		res_container,
		VK_SHADER_STAGE_COMPUTE_BIT, k_light_array_descriptor_set_idx,
		[&](VkDescriptorSet desc_set) {
			m_renderer->write_light_array_descriptor_set(frame_idx, desc_set);
		}
	);

	/* Point lights instance buffer */
	m_pipeline.acquire_and_bind_descriptor_set(
		*m_context,
		cmd_buf,
		res_container,
		VK_SHADER_STAGE_COMPUTE_BIT, k_light_array_movement_buffer_descriptor_set_idx,
		[&](VkDescriptorSet desc_set) {
			write_instance_buffer_descriptor_set(desc_set, point_light_visualizer.m_instances_buffer->m_buffer.m_handle);
		}
	);

	/* Push constants */
	m_pipeline.push_constants(cmd_buf, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants));

	/* */

	vkCmdDispatch(cmd_buf, (uint32_t) glm::ceil(float(point_lights_num) / 512.0f), 1, 1);
}
