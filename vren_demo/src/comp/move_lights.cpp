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
	vren::vk_utils::update_device_only_buffer(tb, buf, buf_data.data(), buf_data.size(), 0);

	return buf;
}

vren_demo::light_array_movement_buf::light_array_movement_buf(
	vren::vk_utils::toolbox const& tb
) :
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

vren::vk_descriptor_set_layout vren_demo::light_array_movement_buf::create_descriptor_set_layout(std::shared_ptr<vren::context> const& ctx)
{
	VkDescriptorSetLayoutBinding bindings[]{
		{ /* point_lights_dirs */
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		}
	};

	VkDescriptorSetLayoutCreateInfo desc_set_layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.bindingCount = (uint32_t) std::size(bindings),
		.pBindings = bindings
	};

	VkDescriptorSetLayout desc_set_layout;
	vren::vk_utils::check(vkCreateDescriptorSetLayout(ctx->m_device, &desc_set_layout_info, nullptr, &desc_set_layout));
	return vren::vk_descriptor_set_layout(ctx, desc_set_layout);
}

vren::vk_descriptor_pool vren_demo::light_array_movement_buf::create_descriptor_pool(std::shared_ptr<vren::context> const& ctx, uint32_t max_sets)
{
	VkDescriptorPoolSize pool_sizes[]{
		{ /* point_lights_dirs */
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
		}
	};

	VkDescriptorPoolCreateInfo desc_pool_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.maxSets = max_sets,
		.poolSizeCount = (uint32_t) std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VkDescriptorPool desc_pool;
	vren::vk_utils::check(vkCreateDescriptorPool(ctx->m_device, &desc_pool_info, nullptr, &desc_pool));
	return vren::vk_descriptor_pool(ctx, desc_pool);
}

// --------------------------------------------------------------------------------------------------------------------------------
// move_lights
// --------------------------------------------------------------------------------------------------------------------------------

vren_demo::move_lights::move_lights(vren::vk_utils::toolbox const& tb) :
	/* light_array_movement_buf */
	m_light_array_movement_buf(tb),
	m_light_array_movement_buf_descriptor_set_layout(
		vren_demo::light_array_movement_buf::create_descriptor_set_layout(tb.m_context)
	),
	m_light_array_movement_buf_descriptor_pool(
		vren_demo::light_array_movement_buf::create_descriptor_pool(tb.m_context, 1)
	),
	/* */
	m_pipeline(
		vren::vk_utils::create_compute_pipeline(
			tb.m_context,
			{ /* Descriptor set layouts */
				m_light_array_movement_buf_descriptor_set_layout.m_handle,
			},
			{ /* Push constants */
				{
					.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
					.offset = 0,
					.size = sizeof(push_constants)
				}
			},
			vren::vk_utils::load_shader_module(tb.m_context, "resources/shaders/move_lights.comp.bin")
		)
	)
{
	VkDescriptorSetAllocateInfo alloc_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = m_light_array_movement_buf_descriptor_pool.m_handle,
		.descriptorSetCount = 1,
		.pSetLayouts = &m_light_array_movement_buf_descriptor_set_layout.m_handle,
	};
	vren::vk_utils::check(vkAllocateDescriptorSets(tb.m_context->m_device, &alloc_info, &m_light_array_movement_buf_descriptor_set));

	m_light_array_movement_buf.write_descriptor_set(m_light_array_movement_buf_descriptor_set);
}

void vren_demo::move_lights::dispatch(
	int frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	push_constants push_const,
	VkDescriptorSet light_array_buf_desc_set
)
{
	// The whole class is used to ensure its members' lifetime (descriptor pools, descriptor set layouts...)
	// is ensured for the frame execution.

	res_container.add_resource(shared_from_this());

	// todo in/out memory barriers

	vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.m_pipeline.m_handle);

	/* Push constants */
	vkCmdPushConstants(
		cmd_buf,
		m_pipeline.m_pipeline_layout.m_handle,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		sizeof(push_constants),
		&push_const
	);

	/* light_array_buf */
	vkCmdBindDescriptorSets(
		cmd_buf,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_pipeline.m_pipeline_layout.m_handle,
		0, 									   // firstSet
		1,									   // descriptorSetCount
		&light_array_buf_desc_set,             // pDescriptorSets
		0,
		nullptr
	);

	/* light_array_movement_buf */
	vkCmdBindDescriptorSets(
		cmd_buf,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		m_pipeline.m_pipeline_layout.m_handle,
		1, 											// firstSet
		1,											// descriptorSetCount
		&m_light_array_movement_buf_descriptor_set, // pDescriptorSets
		0,
		nullptr
	);

	auto max_lights = glm::max(VREN_MAX_POINT_LIGHTS, glm::max(VREN_MAX_DIRECTIONAL_LIGHTS, VREN_MAX_SPOT_LIGHTS));
	vkCmdDispatch(cmd_buf, (uint32_t) (glm::ceil(float(max_lights) / 512.0f) * 512.0f), 0, 0);
}
