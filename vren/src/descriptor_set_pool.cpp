#include "descriptor_set_pool.hpp"

#include <stdexcept>

#include "utils/image.hpp"

void vren::descriptor_set_pool::create_material_layout()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	{ // base color texture
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = VREN_MATERIAL_BASE_COLOR_TEXTURE_BINDING;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	{ // metallic roughness texture
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = VREN_MATERIAL_METALLIC_ROUGHNESS_TEXTURE_BINDING;
		binding.descriptorCount = 1;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.pImmutableSamplers = nullptr;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		bindings.push_back(binding);
	}

	VkDescriptorSetLayoutCreateInfo desc_set_layout_info{};
	desc_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc_set_layout_info.pNext = nullptr;
	desc_set_layout_info.flags = 0;
	desc_set_layout_info.bindingCount = bindings.size();
	desc_set_layout_info.pBindings = bindings.data();

	vren::vk_utils::check(vkCreateDescriptorSetLayout(m_context->m_device, &desc_set_layout_info, nullptr, &m_material_layout));
}

void vren::descriptor_set_pool::create_lights_array_layout()
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

	VkDescriptorSetLayoutCreateInfo desc_set_layout_info{};
	desc_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	desc_set_layout_info.pNext = nullptr;
	desc_set_layout_info.flags = 0;
	desc_set_layout_info.bindingCount = bindings.size();
	desc_set_layout_info.pBindings = bindings.data();

	vren::vk_utils::check(vkCreateDescriptorSetLayout(m_context->m_device, &desc_set_layout_info, nullptr, &m_lights_array_layout));
}

void vren::descriptor_set_pool::decorate_material_descriptor_pool_sizes(
	std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes
)
{
	{ // base color texture
		VkDescriptorPoolSize desc_pool_size{};
		desc_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		desc_pool_size.descriptorCount = 1;
		descriptor_pool_sizes.push_back(desc_pool_size);
	}

	{ // metallic roughness texture
		VkDescriptorPoolSize desc_pool_size{};
		desc_pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		desc_pool_size.descriptorCount = 1;
		descriptor_pool_sizes.push_back(desc_pool_size);
	}
}

void vren::descriptor_set_pool::decorate_lights_array_descriptor_pool_sizes(
	std::vector<VkDescriptorPoolSize>& descriptor_pool_sizes
)
{
	{ // point lights
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_pool_size.descriptorCount = 1;
		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}

	{ // directional lights
		VkDescriptorPoolSize descriptor_pool_size{};
		descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_pool_size.descriptorCount = 1;
		descriptor_pool_sizes.push_back(descriptor_pool_size);
	}
}

void vren::descriptor_set_pool::add_descriptor_pool()
{
	std::vector<VkDescriptorPoolSize> desc_pool_sizes{};
	decorate_material_descriptor_pool_sizes(desc_pool_sizes);
	decorate_lights_array_descriptor_pool_sizes(desc_pool_sizes);

	VkDescriptorPoolCreateInfo desc_pool_info{};
	desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desc_pool_info.pNext = nullptr;
	desc_pool_info.flags = 0;
	desc_pool_info.poolSizeCount = (uint32_t) desc_pool_sizes.size();
	desc_pool_info.pPoolSizes = desc_pool_sizes.data();
	desc_pool_info.maxSets = VREN_DESCRIPTOR_SET_POOL_SIZE;

	VkDescriptorPool desc_pool;
	vren::vk_utils::check(vkCreateDescriptorPool(m_context->m_device, &desc_pool_info, nullptr, &desc_pool));

	m_descriptor_pools.push_back(desc_pool);

	printf("INFO: Created descriptor pool %p (total of: %zu)\n", desc_pool, m_descriptor_pools.size());
	fflush(stdout);

	m_last_pool_allocated_count = 0;
}

vren::descriptor_set_pool::descriptor_set_pool(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{
	create_material_layout();
	create_lights_array_layout();

	add_descriptor_pool();
}

vren::descriptor_set_pool::~descriptor_set_pool()
{
	for (VkDescriptorPool descriptor_pool : m_descriptor_pools)
	{
		vkDestroyDescriptorPool(m_context->m_device, descriptor_pool, nullptr);
	}

	vkDestroyDescriptorSetLayout(m_context->m_device, m_material_layout, nullptr);
	vkDestroyDescriptorSetLayout(m_context->m_device, m_lights_array_layout, nullptr);
}

void vren::descriptor_set_pool::acquire_descriptor_sets(
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
			add_descriptor_pool();
		}

		size_t alloc_count = std::min(descriptor_sets_count - i, VREN_DESCRIPTOR_SET_POOL_SIZE - m_last_pool_allocated_count);

		VkDescriptorPool desc_pool = m_descriptor_pools.back();

		VkDescriptorSetAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.pNext = nullptr;
		alloc_info.descriptorPool = desc_pool;
		alloc_info.descriptorSetCount = alloc_count;
		alloc_info.pSetLayouts = descriptor_set_layouts;

		descriptor_sets_count -= alloc_count;
		m_last_pool_allocated_count += alloc_count;

		vren::vk_utils::check(vkAllocateDescriptorSets(m_context->m_device, &alloc_info, descriptor_sets + i));

		size_t tot_desc_sets = (m_descriptor_pools.size() - 1) * VREN_DESCRIPTOR_SET_POOL_SIZE + m_last_pool_allocated_count;

		printf("INFO: Allocated %zu descriptor set(s) from descriptor pool %p (total of: %zu/%zu)\n", alloc_count, desc_pool, tot_desc_sets, VREN_DESCRIPTOR_SET_POOL_SIZE * m_descriptor_pools.size());
		fflush(stdout);

		i += alloc_count;
	}
}

void vren::descriptor_set_pool::acquire_material_descriptor_set(VkDescriptorSet* descriptor_set)
{
	acquire_descriptor_sets(1, &m_material_layout, descriptor_set);
}

void vren::descriptor_set_pool::acquire_lights_array_descriptor_set(VkDescriptorSet* descriptor_set)
{
	acquire_descriptor_sets(1, &m_lights_array_layout, descriptor_set);
}

void vren::descriptor_set_pool::release_descriptor_set(VkDescriptorSet descriptor_set)
{
	m_descriptor_sets.push(descriptor_set);
}
