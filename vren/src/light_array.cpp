#include "light_array.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// light_array_descriptor_pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::light_array_descriptor_pool::light_array_descriptor_pool(
	std::shared_ptr<vren::context> const& ctx
) :
	vren::descriptor_pool(ctx)
{}

VkDescriptorPool vren::light_array_descriptor_pool::create_descriptor_pool(int max_sets)
{
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, // Point lights
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}, // Directional lights
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}  // Spotlights
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.pNext = nullptr;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool desc_pool;
	vren::vk_utils::check(vkCreateDescriptorPool(m_context->m_device, &pool_info, nullptr, &desc_pool));

	return desc_pool;
}

// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_descriptor_set_layout vren::create_light_array_descriptor_set_layout(std::shared_ptr<vren::context> const& ctx)
{
	VkDescriptorSetLayoutBinding bindings[]{
		{ /* Point lights */
			.binding = VREN_LIGHT_ARRAY_POINT_LIGHT_BUFFER_BINDING,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		},
		{ /* Directional lights */
			.binding = VREN_LIGHT_ARRAY_DIRECTIONAL_LIGHT_BUFFER_BINDING,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		},
		{ /* Spot lights */
			.binding = VREN_LIGHT_ARRAY_SPOT_LIGHT_BUFFER_BINDING,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	VkDescriptorSetLayoutCreateInfo desc_set_layout_info{};
	desc_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc_set_layout_info.pNext = nullptr;
	desc_set_layout_info.flags = 0;
	desc_set_layout_info.bindingCount = std::size(bindings);
	desc_set_layout_info.pBindings = bindings;

	VkDescriptorSetLayout desc_set_layout;
	vren::vk_utils::check(vkCreateDescriptorSetLayout(ctx->m_device, &desc_set_layout_info, nullptr, &desc_set_layout));
	return vren::vk_descriptor_set_layout(ctx, desc_set_layout);
}

void vren::update_light_array_descriptor_set(vren::context const& ctx, vren::light_array const& light_arr, VkDescriptorSet desc_set)
{
	VkDescriptorBufferInfo buffers_info[]{
		{ /* Point lights */
			.buffer = light_arr.m_point_lights_buffer.get_buffer()->m_buffer.m_handle,
			.offset = 0,
			.range = light_arr.m_point_lights_buffer.get_buffer_length()
		},
		{ /* Directional lights */
			.buffer = light_arr.m_directional_lights_buffer.get_buffer()->m_buffer.m_handle,
			.offset = 0,
			.range = light_arr.m_directional_lights_buffer.get_buffer_length()
		},
		{ /* Spot lights */
			.buffer = light_arr.m_spot_lights_buffer.get_buffer()->m_buffer.m_handle,
			.offset = 0,
			.range = light_arr.m_spot_lights_buffer.get_buffer_length()
		}
	};

	VkWriteDescriptorSet desc_set_write{};
	desc_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_write.pNext = nullptr;
	desc_set_write.dstSet = desc_set;
	desc_set_write.dstBinding = VREN_LIGHT_ARRAY_POINT_LIGHT_BUFFER_BINDING;
	desc_set_write.dstArrayElement = 0;
	desc_set_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_set_write.descriptorCount = std::size(buffers_info);
	desc_set_write.pImageInfo = nullptr;
	desc_set_write.pBufferInfo = buffers_info;
	desc_set_write.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(ctx.m_device, 1, &desc_set_write, 0, nullptr);
}
