#include "light.hpp"

#include "base/base.hpp"
#include "context.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::vk_descriptor_set_layout vren::light_array::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding bindings[]{
		{ // Point lights
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		{ // Directional lights
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		{ // Spot lights
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		}
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

vren::vk_descriptor_pool vren::light_array::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[]{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
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

std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::light_array::create_buffers(size_t buffer_size)
{
	return create_array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&](uint32_t idx) {
		return vren::vk_utils::alloc_host_visible_buffer(*m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size, true);
	});
}

std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::light_array::allocate_descriptor_sets()
{
	std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> descriptor_sets;

	auto descriptor_set_layouts = vren::create_array<VkDescriptorSetLayout, VREN_MAX_FRAME_IN_FLIGHT_COUNT>(m_descriptor_set_layout.m_handle);
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = m_descriptor_pool.m_handle,
		.descriptorSetCount = std::size(descriptor_sets),
		.pSetLayouts = descriptor_set_layouts.data()
	};
	VREN_CHECK(vkAllocateDescriptorSets(m_context->m_device, &descriptor_set_alloc_info, descriptor_sets.data()), m_context);

	for (auto const& descriptor_set : descriptor_sets) {
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_set, "light_array");
	}

	return descriptor_sets;
}

vren::light_array::light_array(vren::context const& context) :
	m_context(&context),
	m_descriptor_set_layout(create_descriptor_set_layout()),
	m_descriptor_pool(create_descriptor_pool()),
	m_point_light_buffers(create_buffers(k_point_light_buffer_size)),
	m_directional_light_buffers(create_buffers(k_directional_light_buffer_size)),
	m_spot_light_buffers(create_buffers(k_spot_light_buffer_size)),
	m_descriptor_sets(allocate_descriptor_sets())
{
	for (uint32_t i = 0; i < m_descriptor_sets.size(); i++) {
		write_descriptor_set(i);
	}
}

void vren::light_array::request_point_light_buffer_sync()
{
	m_point_light_buffers_dirty_flags = UINT32_MAX;
}

void vren::light_array::request_directional_light_buffer_sync()
{
	m_directional_light_buffers_dirty_flags = UINT32_MAX;
}

void vren::light_array::request_spot_light_buffer_sync()
{
	m_point_light_buffers_dirty_flags = UINT32_MAX;
}

void vren::light_array::write_descriptor_set(uint32_t frame_idx)
{
	VkDescriptorBufferInfo buffer_info[]{
		{ // Point lights
			.buffer = m_point_light_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range = m_point_light_count > 0 ? m_point_light_count * sizeof(vren::point_light) : 1
		},
		{ // Directional lights
			.buffer = m_directional_light_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range = m_directional_light_count > 0 ? m_directional_light_count * sizeof(vren::directional_light) : 1
		},
		{ // Spot lights
			.buffer = m_spot_light_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range = m_spot_light_count > 0 ? m_spot_light_count * sizeof(vren::spot_light) : 1
		}
	};
	VkWriteDescriptorSet descriptor_set_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptor_sets.at(frame_idx),
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = std::size(buffer_info),
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::light_array::sync_buffers(uint32_t frame_idx)
{
	bool synced = false;

	if ((m_point_light_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_point_light_count > 0) {
			vren::vk_utils::update_host_visible_buffer(*m_context, m_point_light_buffers.at(frame_idx), m_point_lights.data(), m_point_light_count * sizeof(vren::point_light), 0);
		}
		synced = true;

		m_point_light_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	if ((m_directional_light_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_directional_light_count > 0) {
			vren::vk_utils::update_host_visible_buffer(*m_context, m_directional_light_buffers.at(frame_idx), m_directional_lights.data(), m_directional_light_count * sizeof(vren::directional_light), 0);
		}
		synced = true;

		m_directional_light_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	if ((m_spot_light_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_spot_light_count > 0) {
			vren::vk_utils::update_host_visible_buffer(*m_context, m_spot_light_buffers.at(frame_idx), m_spot_lights.data(), m_spot_light_count * sizeof(vren::spot_light), 0);
		}
		synced = true;

		m_spot_light_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	if (synced)
	{
		write_descriptor_set(frame_idx);
	}
}

VkDescriptorSet vren::light_array::get_descriptor_set(uint32_t frame_idx) const
{
	return m_descriptor_sets.at(frame_idx);
}
