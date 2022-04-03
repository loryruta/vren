#include "material.hpp"

#include "context.hpp"
#include "utils/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Material
// --------------------------------------------------------------------------------------------------------------------------------

vren::material::material(vren::context const& ctx) :
	m_base_color_texture(ctx.m_toolbox->m_white_texture),
	m_metallic_roughness_texture(ctx.m_toolbox->m_white_texture),
	m_base_color_factor(1.0f),
	m_metallic_factor(0.0f),
	m_roughness_factor(0.0f)
{}

// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_descriptor_set_layout vren::create_material_descriptor_set_layout(vren::context const& ctx)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	// Base color texture
	bindings.push_back({
		.binding = VREN_MATERIAL_BASE_COLOR_TEXTURE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
	});

	// Metallic roughness texture
	bindings.push_back({
		.binding = VREN_MATERIAL_METALLIC_ROUGHNESS_TEXTURE_BINDING,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr
    });

	VkDescriptorSetLayoutCreateInfo desc_set_layout_info{};
	desc_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc_set_layout_info.pNext = nullptr;
	desc_set_layout_info.flags = 0;
	desc_set_layout_info.bindingCount = bindings.size();
	desc_set_layout_info.pBindings = bindings.data();

	VkDescriptorSetLayout desc_set_layout;
	vren::vk_utils::check(vkCreateDescriptorSetLayout(ctx.m_device, &desc_set_layout_info, nullptr, &desc_set_layout));
	return vren::vk_descriptor_set_layout(ctx, desc_set_layout);
}

void vren::update_material_descriptor_set(vren::context const& ctx, vren::material const& mat, VkDescriptorSet desc_set)
{
	std::vector<VkWriteDescriptorSet> desc_set_writes;
	VkWriteDescriptorSet desc_set_write{};

	// Base color texture
	VkDescriptorImageInfo base_col_tex_info{};
	base_col_tex_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	base_col_tex_info.imageView = mat.m_base_color_texture->m_image_view.m_handle;
	base_col_tex_info.sampler = mat.m_base_color_texture->m_sampler.m_handle;

	desc_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_write.pNext = nullptr;
	desc_set_write.dstSet = desc_set;
	desc_set_write.dstBinding = VREN_MATERIAL_BASE_COLOR_TEXTURE_BINDING;
	desc_set_write.dstArrayElement = 0;
	desc_set_write.descriptorCount = 1;
	desc_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_set_write.pImageInfo = &base_col_tex_info;
	desc_set_write.pBufferInfo = nullptr;
	desc_set_write.pTexelBufferView = nullptr;

	desc_set_writes.push_back(desc_set_write);

	// Metallic roughness texture
	VkDescriptorImageInfo met_rough_tex_info{};
	met_rough_tex_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	met_rough_tex_info.imageView = mat.m_metallic_roughness_texture->m_image_view.m_handle;
	met_rough_tex_info.sampler = mat.m_metallic_roughness_texture->m_sampler.m_handle;

	desc_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_write.pNext = nullptr;
	desc_set_write.dstSet = desc_set;
	desc_set_write.dstBinding = VREN_MATERIAL_METALLIC_ROUGHNESS_TEXTURE_BINDING;
	desc_set_write.dstArrayElement = 0;
	desc_set_write.descriptorCount = 1;
	desc_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_set_write.pImageInfo = &met_rough_tex_info;
	desc_set_write.pBufferInfo = nullptr;
	desc_set_write.pTexelBufferView = nullptr;

	desc_set_writes.push_back(desc_set_write);

	vkUpdateDescriptorSets(ctx.m_device, desc_set_writes.size(), desc_set_writes.data(), 0, nullptr);
}
