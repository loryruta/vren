#include "material.hpp"

vren::material::material(std::shared_ptr<vren::context> const& ctx) :
	m_base_color_texture(ctx->m_white_texture),
	m_metallic_roughness_texture(ctx->m_white_texture),
	m_base_color_factor(1.0f),
	m_metallic_factor(0.0f),
	m_roughness_factor(0.0f)
{
}

void vren::material_manager::update_material_descriptor_set(vren::context const& ctx, vren::material const& material, VkDescriptorSet descriptor_set)
{
	std::vector<VkWriteDescriptorSet> desc_set_writes;
	VkWriteDescriptorSet desc_set_write{};

	// Base color
	VkDescriptorImageInfo base_col_tex_info{};
	base_col_tex_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	base_col_tex_info.imageView = material.m_base_color_texture->m_image_view->m_handle;
	base_col_tex_info.sampler = material.m_base_color_texture->m_sampler->m_handle;

	desc_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_write.pNext = nullptr;
	desc_set_write.dstSet = descriptor_set;
	desc_set_write.dstBinding = VREN_MATERIAL_BASE_COLOR_TEXTURE_BINDING;
	desc_set_write.dstArrayElement = 0;
	desc_set_write.descriptorCount = 1;
	desc_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_set_write.pImageInfo = &base_col_tex_info;
	desc_set_write.pBufferInfo = nullptr;
	desc_set_write.pTexelBufferView = nullptr;

	desc_set_writes.push_back(desc_set_write);

	// Metallic/roughness texture
	VkDescriptorImageInfo met_rough_tex_info{};
	met_rough_tex_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	met_rough_tex_info.imageView = material.m_metallic_roughness_texture->m_image_view->m_handle;
	met_rough_tex_info.sampler = material.m_metallic_roughness_texture->m_sampler->m_handle;

	desc_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_write.pNext = nullptr;
	desc_set_write.dstSet = descriptor_set;
	desc_set_write.dstBinding = VREN_MATERIAL_METALLIC_ROUGHNESS_TEXTURE_BINDING;
	desc_set_write.dstArrayElement = 0;
	desc_set_write.descriptorCount = 1;
	desc_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc_set_write.pImageInfo = &met_rough_tex_info;
	desc_set_write.pBufferInfo = nullptr;
	desc_set_write.pTexelBufferView = nullptr;

	desc_set_writes.push_back(desc_set_write);

	//
	vkUpdateDescriptorSets(ctx.m_device, desc_set_writes.size(), desc_set_writes.data(), 0, nullptr);
}
