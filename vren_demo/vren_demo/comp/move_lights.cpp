#include "move_lights.hpp"

#include <glm/gtc/random.hpp>

#include <vren/vk_helpers/misc.hpp>

// --------------------------------------------------------------------------------------------------------------------------------
// light_array_movement_buf
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_utils::buffer create_point_lights_dirs(vren::context const& ctx)
{
	auto buf = vren::vk_utils::alloc_device_only_buffer(ctx, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VREN_MAX_POINT_LIGHTS * sizeof(glm::vec4));

	std::vector<glm::vec4> buf_data(VREN_MAX_POINT_LIGHTS);
	for (int i = 0; i < VREN_MAX_POINT_LIGHTS; i++) {
		buf_data[i] = glm::vec4(glm::sphericalRand(1.f), 1.f);
	}
	vren::vk_utils::update_device_only_buffer(ctx, buf, buf_data.data(), VREN_MAX_POINT_LIGHTS * sizeof(glm::vec4), 0);

	return buf;
}

vren_demo::light_array_movement_buf::light_array_movement_buf(vren::context const& ctx) :
	m_context(&ctx),
	m_point_lights_dirs(create_point_lights_dirs(ctx))
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

vren_demo::move_lights::move_lights(vren::context const& ctx, vren::renderer& renderer) :
	m_context(&ctx),
	m_renderer(&renderer),

	m_light_array_movement_buf(ctx),
	m_pipeline(vren::vk_utils::create_compute_pipeline(
		ctx,
		vren::vk_utils::load_and_describe_shader(ctx, "resources/shaders/move_lights.comp.bin")
	))
{}

void vren_demo::move_lights::dispatch(
	int frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	move_lights::push_constants push_constants
)
{
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

	/* Light array movement buffer */
	m_pipeline.acquire_and_bind_descriptor_set(
		*m_context,
		cmd_buf,
		res_container,
		VK_SHADER_STAGE_COMPUTE_BIT, k_light_array_movement_buffer_descriptor_set_idx,
		[&](VkDescriptorSet desc_set) {
			m_light_array_movement_buf.write_descriptor_set(desc_set);
		}
	);

	/* Push constants */
	m_pipeline.push_constants(cmd_buf, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(move_lights::push_constants));

	/* */
	auto max_lights = glm::max(VREN_MAX_POINT_LIGHTS, glm::max(VREN_MAX_DIRECTIONAL_LIGHTS, VREN_MAX_SPOT_LIGHTS));
	vkCmdDispatch(cmd_buf, (uint32_t) glm::ceil(float(max_lights) / 512.0f), 1, 1);
}
