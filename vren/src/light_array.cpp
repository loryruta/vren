#include "light_array.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// light_array_descriptor_pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::light_array_descriptor_pool::light_array_descriptor_pool(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<vren::vk_descriptor_set_layout> const& desc_set_layout
) :
	vren::descriptor_pool(ctx, desc_set_layout)
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
	vren::vk_utils::check(vkCreateDescriptorSetLayout(ctx->m_device, &desc_set_layout_info, nullptr, &desc_set_layout));
	return vren::vk_descriptor_set_layout(ctx, desc_set_layout);
}
