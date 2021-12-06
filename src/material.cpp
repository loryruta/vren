#include "material.hpp"

#include "renderer.hpp"

#define VREN_MATERIAL_DESCRIPTOR_POOL_SIZE 32

VkDescriptorSetLayout vren::material_descriptor_set_pool::_create_descriptor_set_layout() const
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	{ // Material colors
		VkDescriptorSetLayoutBinding colors_binding{};
		colors_binding.binding = VREN_MATERIAL_COLORS_BINDING;
		colors_binding.descriptorCount = 2;
		colors_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		colors_binding.pImmutableSamplers = nullptr;
		colors_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(colors_binding);
	}

	{ // Material textures
		VkDescriptorSetLayoutBinding textures_binding{};
		textures_binding.binding = VREN_MATERIAL_TEXTURES_BINDING;
		textures_binding.descriptorCount = 2;
		textures_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textures_binding.pImmutableSamplers = nullptr;
		textures_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(textures_binding);
	}

	VkDescriptorSetLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = bindings.size();
	create_info.pBindings = bindings.data();

	VkDescriptorSetLayout descriptor_set_layout;
	if (vkCreateDescriptorSetLayout(m_renderer.m_device, &create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}
	return descriptor_set_layout;
}

void vren::material_descriptor_set_pool::_add_descriptor_pool()
{
	// Descriptor pool
	std::vector<VkDescriptorPoolSize> descriptor_pool_sizes{};

	{ // Material colors
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_pool_size.descriptorCount = 2;

		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}

	{ // Material textures
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_pool_size.descriptorCount = 2;

		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}

	VkDescriptorPoolCreateInfo descriptor_pool_info{};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.poolSizeCount = (uint32_t) descriptor_pool_sizes.size();
	descriptor_pool_info.pPoolSizes = descriptor_pool_sizes.data();
	descriptor_pool_info.maxSets = VREN_MATERIAL_DESCRIPTOR_POOL_SIZE;

	VkDescriptorPool descriptor_pool;
	if (vkCreateDescriptorPool(m_renderer.m_device, &descriptor_pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}

	m_descriptor_pools.push_back(descriptor_pool);

	m_last_pool_allocated_count = 0;
}

vren::material_descriptor_set_pool::material_descriptor_set_pool(vren::renderer& renderer) :
	m_renderer(renderer)
{
	m_descriptor_set_layout = _create_descriptor_set_layout();

	_add_descriptor_pool();
}

vren::material_descriptor_set_pool::~material_descriptor_set_pool()
{
	for (VkDescriptorPool descriptor_pool : m_descriptor_pools)
	{
		vkDestroyDescriptorPool(m_renderer.m_device, descriptor_pool, nullptr);
	}

	vkDestroyDescriptorSetLayout(m_renderer.m_device, m_descriptor_set_layout, nullptr);
}

void vren::material_descriptor_set_pool::acquire_descriptor_sets(size_t descriptor_sets_count, VkDescriptorSet* descriptor_sets)
{
	size_t i = 0;

	for (; i < descriptor_sets_count && !m_descriptor_sets.empty(); i++)
	{
		descriptor_sets[i] = m_descriptor_sets.front();
		m_descriptor_sets.pop();
	}

	while (i < descriptor_sets_count)
	{
		if (m_last_pool_allocated_count >= 32)
		{
			_add_descriptor_pool();
		}

		size_t alloc_count = std::min(descriptor_sets_count - i, 32 - m_last_pool_allocated_count);

		std::vector<VkDescriptorSetLayout> descriptor_set_layouts(alloc_count, m_descriptor_set_layout);

		VkDescriptorSetAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = m_descriptor_pools.back();
		alloc_info.descriptorSetCount = alloc_count;
		alloc_info.pSetLayouts = descriptor_set_layouts.data();

		descriptor_sets_count -= alloc_count;

		if (vkAllocateDescriptorSets(m_renderer.m_device, &alloc_info, descriptor_sets + i) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate descriptor sets");
		}

		i += alloc_count;
	}
}

void vren::material_descriptor_set_pool::update_descriptor_set(vren::material const* material, VkDescriptorSet descriptor_set)
{
	std::vector<VkDescriptorImageInfo> images_info{};

	{ // Diffuse texture
		auto& diff_tex = material->m_diffuse_texture;

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = diff_tex.m_image_view.m_handle;
		image_info.sampler = diff_tex.m_sampler_handle;

		images_info.push_back(image_info);
	}

	{ // Specular texture
		auto& spec_tex = material->m_specular_texture;

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = spec_tex.m_image_view.m_handle;
		image_info.sampler = spec_tex.m_sampler_handle;

		images_info.push_back(image_info);
	}

	VkWriteDescriptorSet descriptor_set_write{};
	descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_set_write.dstSet = descriptor_set;
	descriptor_set_write.dstBinding = VREN_MATERIAL_TEXTURES_BINDING;
	descriptor_set_write.dstArrayElement = 0;
	descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_set_write.descriptorCount = images_info.size();
	descriptor_set_write.pImageInfo = images_info.data();

	vkUpdateDescriptorSets(m_renderer.m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::material_descriptor_set_pool::release_descriptor_set(VkDescriptorSet descriptor_set)
{
	m_descriptor_sets.push(descriptor_set);
}

vren::material::material(vren::renderer& renderer)
{
	m_diffuse_color  = glm::vec4(1.0f);
	m_specular_color = glm::vec4(1.0f);

	m_specular_texture = renderer.m_white_texture;
	m_diffuse_texture  = renderer.m_white_texture;
}
