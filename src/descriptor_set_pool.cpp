#include "descriptor_set_pool.hpp"

#include <stdexcept>

#include "renderer.hpp"

void vren::descriptor_set_pool::_create_material_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	{ // Albedo texture
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = VREN_MATERIAL_ALBEDO_TEXTURE_BINDING;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	{ // Material struct
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = VREN_MATERIAL_MATERIAL_STRUCT_BINDING;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = bindings.size();
	create_info.pBindings = bindings.data();

	vren::vk_utils::check(vkCreateDescriptorSetLayout(m_renderer.m_device, &create_info, nullptr, &m_material_layout));
}

void vren::descriptor_set_pool::_create_lights_array_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	{ // Point lights
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = VREN_POINT_LIGHTS_BINDING;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	{ // Directional lights
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = VREN_DIRECTIONAL_LIGHTS_BINDING;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create_info.bindingCount = bindings.size();
	create_info.pBindings = bindings.data();

	vren::vk_utils::check(vkCreateDescriptorSetLayout(m_renderer.m_device, &create_info, nullptr, &m_lights_array_layout));
}

void vren::descriptor_set_pool::_decorate_material_descriptor_pool_sizes(
	std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes
)
{
	{ // Albedo texture
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_pool_size.descriptorCount = 1;

		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}

	{ // Material textures
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_pool_size.descriptorCount = 1;

		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}
}

void vren::descriptor_set_pool::_decorate_lights_array_descriptor_pool_sizes(
	std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes
)
{
	{ // Point lights
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_pool_size.descriptorCount = 1;

		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}

	{ // Directional lights
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_pool_size.descriptorCount = 1;

		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}
}

void vren::descriptor_set_pool::_add_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> descriptor_pool_sizes{};
	_decorate_material_descriptor_pool_sizes(descriptor_pool_sizes);
	_decorate_lights_array_descriptor_pool_sizes(descriptor_pool_sizes);

	VkDescriptorPoolCreateInfo descriptor_pool_info{};
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_info.poolSizeCount = (uint32_t) descriptor_pool_sizes.size();
	descriptor_pool_info.pPoolSizes = descriptor_pool_sizes.data();
	descriptor_pool_info.maxSets = VREN_DESCRIPTOR_SET_POOL_SIZE;

	VkDescriptorPool descriptor_pool;
	vren::vk_utils::check(vkCreateDescriptorPool(m_renderer.m_device, &descriptor_pool_info, nullptr, &descriptor_pool));

	m_descriptor_pools.push_back(descriptor_pool);

	m_last_pool_allocated_count = 0;
}

vren::descriptor_set_pool::descriptor_set_pool(vren::renderer& renderer) :
	m_renderer(renderer)
{
	_create_material_layout();
	_create_lights_array_layout();

	_add_descriptor_pool();
}

vren::descriptor_set_pool::~descriptor_set_pool()
{
	for (VkDescriptorPool descriptor_pool : m_descriptor_pools)
	{
		vkDestroyDescriptorPool(m_renderer.m_device, descriptor_pool, nullptr);
	}

	vkDestroyDescriptorSetLayout(m_renderer.m_device, m_material_layout, nullptr);
	vkDestroyDescriptorSetLayout(m_renderer.m_device, m_lights_array_layout, nullptr);
}

void vren::descriptor_set_pool::_acquire_descriptor_sets(
	size_t descriptor_sets_count,
	VkDescriptorSetLayout* descriptor_set_layouts,
	VkDescriptorSet* descriptor_sets
)
{
	size_t i = 0;

	for (; i < descriptor_sets_count && !m_descriptor_sets.empty(); i++)
	{
		descriptor_sets[i] = m_descriptor_sets.front();
		m_descriptor_sets.pop();
	}

	while (i < descriptor_sets_count)
	{
		if (m_last_pool_allocated_count >= VREN_DESCRIPTOR_SET_POOL_SIZE)
		{
			_add_descriptor_pool();
		}

		size_t alloc_count = std::min(descriptor_sets_count - i, VREN_DESCRIPTOR_SET_POOL_SIZE - m_last_pool_allocated_count);

		VkDescriptorSetAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = m_descriptor_pools.back();
		alloc_info.descriptorSetCount = alloc_count;
		alloc_info.pSetLayouts = descriptor_set_layouts;

		descriptor_sets_count -= alloc_count;

		vren::vk_utils::check(vkAllocateDescriptorSets(m_renderer.m_device, &alloc_info, descriptor_sets + i));

		i += alloc_count;
	}
}

void vren::descriptor_set_pool::acquire_material_descriptor_set(VkDescriptorSet* descriptor_set)
{
	_acquire_descriptor_sets(1, &m_material_layout, descriptor_set);
}

void vren::descriptor_set_pool::acquire_lights_array_descriptor_set(VkDescriptorSet* descriptor_set)
{
	_acquire_descriptor_sets(1, &m_lights_array_layout, descriptor_set);
}

void vren::descriptor_set_pool::release_descriptor_set(VkDescriptorSet descriptor_set)
{
	m_descriptor_sets.push(descriptor_set);
}
