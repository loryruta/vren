#include "move_lights.hpp"

#include <glm/gtc/random.hpp>

#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// light_array_movement_buf
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::buffer _create_point_lights_dirs(vren::vk_utils::toolbox const& tb)
{
	auto buf = vren::vk_utils::alloc_device_only_buffer(tb.m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VREN_MAX_POINT_LIGHTS * sizeof(glm::vec4));

	std::vector<glm::vec4> buf_data(VREN_MAX_POINT_LIGHTS);
	for (int i = 0; i < VREN_MAX_POINT_LIGHTS; i++) {
		buf_data[i] = glm::vec4(glm::sphericalRand(1.f), 1.f);
	}
	vren::vk_utils::update_device_only_buffer(tb, buf, buf_data.data(), VREN_MAX_POINT_LIGHTS * sizeof(glm::vec4), 0);

	return buf;
}

vren_demo::light_array_movement_buf::light_array_movement_buf(
	vren::vk_utils::toolbox const& tb
) :
	m_context(tb.m_context),

	m_point_lights_dirs(_create_point_lights_dirs(tb))
{}

void vren_demo::light_array_movement_buf::write_descriptor_set(VkDescriptorSet desc_set)
{
	VkDescriptorBufferInfo buffers_info[]{
		{ /* point_lights_dirs */
			.buffer = m_point_lights_dirs.m_buffer.m_handle,
			.offset = 0,
			.range  = VK_WHOLE_SIZE
		}
	};

	VkWriteDescriptorSet desc_set_writes[]{
		{ /* point_lights_dirs */
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = desc_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.pImageInfo = nullptr,
			.pBufferInfo = buffers_info,
			.pTexelBufferView = nullptr,
		},
	};
	vkUpdateDescriptorSets(m_context->m_device, (uint32_t) std::size(desc_set_writes), desc_set_writes, 0, nullptr);
}

// --------------------------------------------------------------------------------------------------------------------------------
// move_lights
// --------------------------------------------------------------------------------------------------------------------------------

vren_demo::move_lights::move_lights(
	vren::renderer& renderer
) :
	m_renderer(&renderer),

	m_light_array_movement_buf(*renderer.m_toolbox),
	m_shader(std::make_shared<vren::vk_utils::self_described_shader>(
			vren::vk_utils::load_and_describe_shader(renderer.m_toolbox->m_context, "resources/shaders/move_lights.comp.bin")
	)),
	m_pipeline(
		vren::vk_utils::create_compute_pipeline(renderer.m_toolbox->m_context, m_shader)
	)
{}

void vren_demo::move_lights::dispatch(
	int frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	push_constants push_const
)
{
	auto& tb = m_renderer->m_toolbox;

	m_pipeline.bind(cmd_buf);

	auto light_arr_layout = m_shader->get_descriptor_set_layout(k_light_arr_desc_set);
	auto light_arr_mov_buf_layout = m_shader->get_descriptor_set_layout(k_light_arr_mov_buf_desc_set);

	/* light_array */
	auto light_arr_desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		tb->m_descriptor_pool->acquire(light_arr_layout)
	);
	m_renderer->write_light_array_descriptor_set(frame_idx, light_arr_desc_set->m_handle.m_descriptor_set);
	m_pipeline.bind_descriptor_set(cmd_buf, 0, light_arr_desc_set->m_handle.m_descriptor_set);
	res_container.add_resource(light_arr_desc_set);

	/* light_array_movement_buf */
	auto light_arr_mov_buf_desc_set = std::make_shared<vren::pooled_vk_descriptor_set>(
		tb->m_descriptor_pool->acquire(light_arr_mov_buf_layout)
	);
	m_light_array_movement_buf.write_descriptor_set(light_arr_mov_buf_desc_set->m_handle.m_descriptor_set);
	m_pipeline.bind_descriptor_set(cmd_buf, 1, light_arr_mov_buf_desc_set->m_handle.m_descriptor_set);
	res_container.add_resource(light_arr_mov_buf_desc_set);

	/* Push constants */
	m_pipeline.push_constants(cmd_buf, VK_SHADER_STAGE_COMPUTE_BIT, &push_const, sizeof(push_const));

	auto max_lights = glm::max(VREN_MAX_POINT_LIGHTS, glm::max(VREN_MAX_DIRECTIONAL_LIGHTS, VREN_MAX_SPOT_LIGHTS));
	vkCmdDispatch(cmd_buf, (uint32_t) glm::ceil(float(max_lights) / 512.0f), 1, 1);
}
