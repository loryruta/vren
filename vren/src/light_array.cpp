#include "light_array.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_descriptor_set_layout vren::create_light_array_descriptor_set_layout(vren::context const& ctx)
{
	VkDescriptorSetLayoutBinding bindings[]{
		{ /* Point lights */
			.binding = VREN_LIGHT_ARRAY_POINT_LIGHT_BUFFER_BINDING,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		{ /* Directional lights */
			.binding = VREN_LIGHT_ARRAY_DIRECTIONAL_LIGHT_BUFFER_BINDING,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		{ /* Spot lights */
			.binding = VREN_LIGHT_ARRAY_SPOT_LIGHT_BUFFER_BINDING,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
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
	vren::vk_utils::check(vkCreateDescriptorSetLayout(ctx.m_device, &desc_set_layout_info, nullptr, &desc_set_layout));
	return vren::vk_descriptor_set_layout(ctx, desc_set_layout);
}
