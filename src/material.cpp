#include "material.hpp"

#include "renderer.hpp"

vren::material::material(vren::renderer& renderer) :
	m_renderer(renderer)
{
	m_albedo_texture = renderer.m_white_texture;

	m_metallic  = 0;
	m_roughness = 0;
}

vren::material::~material()
{
	if (m_albedo_texture != m_renderer.m_white_texture) // todo delegate the texture destruction to a specialized class (texture_manager)
	{
		vren::destroy_texture(m_renderer, m_albedo_texture);
	}
}

struct material_struct
{
	float m_metallic;
	float m_roughness;
};

vren::material_manager::material_manager(vren::renderer& renderer) :
	m_renderer(renderer)
{
	size_t min_uniform_buffer_offset_alignment = m_renderer.m_physical_device_properties.limits.minUniformBufferOffsetAlignment;
	m_material_struct_aligned_size =
		(size_t) glm::ceil((float) sizeof(material_struct) / (float) min_uniform_buffer_offset_alignment) * min_uniform_buffer_offset_alignment;

	m_uniform_buffer_size = 0;
}

vren::material_manager::~material_manager()
{
	m_renderer.m_gpu_allocator->destroy_buffer_if_any(
		m_uniform_buffer
	);
}

vren::material* vren::material_manager::create_material()
{
	auto& material = m_materials.emplace_back(std::make_unique<vren::material>(m_renderer));

	material->m_idx = m_materials.size() - 1;

	printf("Creating material: %d\n", material->m_idx);

	return material.get();
}

vren::material* vren::material_manager::get_material(uint32_t idx)
{
	return m_materials.at(idx).get();
}

void vren::material_manager::upload_device_buffer()
{
	if (m_materials.empty())
	{
		return;
	}

	if (m_materials.size() * m_material_struct_aligned_size > m_uniform_buffer_size)
	{
		m_renderer.m_gpu_allocator->destroy_buffer_if_any(
			m_uniform_buffer
		);

		size_t new_size =
			(size_t) glm::ceil((float) (m_materials.size() * m_material_struct_aligned_size) / (float) 256) * 256;

		m_renderer.m_gpu_allocator->alloc_host_visible_buffer(
			m_uniform_buffer,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			new_size,
			false
		);

		m_uniform_buffer_size = new_size;
	}

	std::vector<uint8_t> uniform_buffer_data;

	uniform_buffer_data.resize(m_materials.size() * m_material_struct_aligned_size);

	for (int i = 0; i < m_materials.size(); i++)
	{
		auto& material = m_materials.at(i);

		auto slot = reinterpret_cast<material_struct*>(uniform_buffer_data.data() + i * m_material_struct_aligned_size);
		slot->m_metallic  = material->m_metallic;
		slot->m_roughness = material->m_roughness;
	}

	m_renderer.m_gpu_allocator->update_device_only_buffer(
		m_uniform_buffer,
		uniform_buffer_data.data(),
		uniform_buffer_data.size(),
		0
	);
}

void vren::material_manager::update_material_descriptor_set(vren::material const* material, VkDescriptorSet descriptor_set)
{
	{ // Albedo texture
		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = material->m_albedo_texture.m_image_view.m_handle;
		image_info.sampler = material->m_albedo_texture.m_sampler_handle;

		VkWriteDescriptorSet descriptor_set_write{};
		descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_set_write.dstSet = descriptor_set;
		descriptor_set_write.dstBinding = VREN_MATERIAL_ALBEDO_TEXTURE_BINDING;
		descriptor_set_write.dstArrayElement = 0;
		descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_set_write.descriptorCount = 1;
		descriptor_set_write.pImageInfo = &image_info;

		vkUpdateDescriptorSets(m_renderer.m_device, 1, &descriptor_set_write, 0, nullptr);
	}

	{ // Material struct
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = m_uniform_buffer.m_buffer;
		buffer_info.offset = material->m_idx * m_material_struct_aligned_size;
		buffer_info.range  = m_material_struct_aligned_size;

		VkWriteDescriptorSet descriptor_set_write{};
		descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_set_write.dstSet = descriptor_set;
		descriptor_set_write.dstBinding = VREN_MATERIAL_MATERIAL_STRUCT_BINDING;
		descriptor_set_write.dstArrayElement = 0;
		descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_set_write.descriptorCount = 1;
		descriptor_set_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(m_renderer.m_device, 1, &descriptor_set_write, 0, nullptr);
	}
}
