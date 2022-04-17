#include "material.hpp"

#include "base/base.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::material_manager::material_manager(vren::context const& context) :
	m_context(&context),
	m_descriptor_set_layout(create_descriptor_set_layout()),
	m_descriptor_pool(create_descriptor_pool()),
	m_buffers(create_buffers()),
	m_descriptor_sets(allocate_descriptor_sets())
{
	for (uint32_t i = 0; i < m_descriptor_sets.size(); i++) {
		write_descriptor_set(i);
	}
}

vren::vk_descriptor_set_layout vren::material_manager::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding bindings[]{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
	};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.bindingCount = std::size(bindings),
		.pBindings = bindings
	};
	VkDescriptorSetLayout descriptor_set_layout;
	VREN_CHECK(vkCreateDescriptorSetLayout(m_context->m_device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout), m_context);
	return vren::vk_descriptor_set_layout(*m_context, descriptor_set_layout);
}

vren::vk_descriptor_pool vren::material_manager::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[]{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VREN_MAX_FRAME_IN_FLIGHT_COUNT },
	};
	VkDescriptorPoolCreateInfo descriptor_pool_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.maxSets = VREN_MAX_FRAME_IN_FLIGHT_COUNT,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	VkDescriptorPool descriptor_pool;
	VREN_CHECK(vkCreateDescriptorPool(m_context->m_device, &descriptor_pool_info, nullptr, &descriptor_pool), m_context);
	return vren::vk_descriptor_pool(*m_context, descriptor_pool);
}

std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::material_manager::create_buffers()
{
	return vren::create_array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&]() {
		return vren::vk_utils::alloc_device_only_buffer(*m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, k_max_material_buffer_size);
	});
}

std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::material_manager::allocate_descriptor_sets()
{
	std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> descriptor_sets;

	std::array<VkDescriptorSetLayout, VREN_MAX_FRAME_IN_FLIGHT_COUNT> descriptor_set_layouts;
	std::fill(descriptor_set_layouts.begin(), descriptor_set_layouts.end(), m_descriptor_set_layout.m_handle);

	VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = m_descriptor_pool.m_handle,
		.descriptorSetCount = std::size(descriptor_sets),
		.pSetLayouts = descriptor_set_layouts.data()
	};
	VREN_CHECK(vkAllocateDescriptorSets(m_context->m_device, &descriptor_set_alloc_info, descriptor_sets.data()), m_context);

	for (auto const& descriptor_set : descriptor_sets) {
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_set, "material_manager");
	}

	return descriptor_sets;
}

void vren::material_manager::write_descriptor_set(uint32_t frame_idx)
{
	VkDescriptorBufferInfo buffer_info{
		.buffer = m_buffers.at(frame_idx).m_buffer.m_handle,
		.offset = 0,
		.range = m_material_count > 0 ? m_material_count * sizeof(vren::material) : 1
	};
	VkWriteDescriptorSet descriptor_set_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptor_sets.at(frame_idx),
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::material_manager::request_buffer_sync()
{
	m_buffers_dirty_flags = UINT32_MAX;
}

void vren::material_manager::sync_buffer(uint32_t frame_idx)
{
	if ((m_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_material_count > 0) {
			vren::vk_utils::update_device_only_buffer(*m_context, m_buffers.at(frame_idx), m_materials.data(), m_material_count * sizeof(vren::material), 0);
		}
		write_descriptor_set(frame_idx);

		m_buffers_dirty_flags &= ~(1 << frame_idx);
	}
}

VkDescriptorSet vren::material_manager::get_descriptor_set(uint32_t frame_idx) const
{
	return m_descriptor_sets.at(frame_idx);
}
