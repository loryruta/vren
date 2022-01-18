#include "material.hpp"

#include "renderer.hpp"

vren::material::material(vren::renderer& renderer) :
	m_base_color_factor(1.0f),
	m_base_color_texture(renderer.m_white_texture),
	m_metallic_factor(0.0f),
	m_roughness_factor(0.0f),
	m_metallic_roughness_texture(renderer.m_black_texture)
{
}

void vren::material_manager::update_material_descriptor_set(vren::renderer const& renderer, vren::material const& material, VkDescriptorSet descriptor_set)
{
	{ // base color texture
		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = material.m_base_color_texture->m_image_view->m_handle;
		image_info.sampler = material.m_base_color_texture->m_sampler->m_handle;

		VkWriteDescriptorSet descriptor_set_write{};
		descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_set_write.dstSet = descriptor_set;
		descriptor_set_write.dstBinding = VREN_MATERIAL_BASE_COLOR_TEXTURE_BINDING;
		descriptor_set_write.dstArrayElement = 0;
		descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_set_write.descriptorCount = 1;
		descriptor_set_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(renderer.m_device, 1, &descriptor_set_write, 0, nullptr);
	}

	{ // metallic roughness texture
		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = material.m_metallic_roughness_texture->m_image_view->m_handle;
		image_info.sampler = material.m_metallic_roughness_texture->m_sampler->m_handle;

		VkWriteDescriptorSet descriptor_set_write{};
		descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_set_write.dstSet = descriptor_set;
		descriptor_set_write.dstBinding = VREN_MATERIAL_METALLIC_ROUGHNESS_TEXTURE_BINDING;
		descriptor_set_write.dstArrayElement = 0;
		descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_set_write.descriptorCount = 1;
		descriptor_set_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(renderer.m_device, 1, &descriptor_set_write, 0, nullptr);
	}
}
