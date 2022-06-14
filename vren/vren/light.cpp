#include "light.hpp"

#include "base/base.hpp"
#include "context.hpp"
#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::light_array::light_array(vren::context const& context) :
	m_context(&context),

	m_point_light_buffer(create_buffer(k_point_light_buffer_size, "point_light_buffer")),
	m_directional_light_buffer(create_buffer(k_directional_light_buffer_size, "directional_light_buffer")),
	m_spot_light_buffer(create_buffer(k_spot_light_buffer_size, "spot_light_buffer"))
{
}

vren::vk_utils::buffer vren::light_array::create_buffer(size_t buffer_size, std::string const& buffer_name)
{
	vren::vk_utils::buffer buffer = vren::vk_utils::alloc_host_visible_buffer(*m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size, true); // TODO required to be DEVICE_LOCAL!

	vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, buffer_name.c_str());

	return buffer;
}

void vren::light_array::write_descriptor_set(VkDescriptorSet descriptor_set) const 
{
	VkDescriptorBufferInfo buffer_info;
	VkWriteDescriptorSet descriptor_set_write;

	// Point lights
	buffer_info = {
		.buffer = m_point_light_buffer.m_buffer.m_handle,
		.offset = 0,
		.range = m_point_light_count > 0 ? m_point_light_count * sizeof(vren::point_light) : 1
	};
	descriptor_set_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);

	// Directional lights
	buffer_info = {
		.buffer = m_directional_light_buffer.m_buffer.m_handle,
		.offset = 0,
		.range = m_directional_light_count > 0 ? m_directional_light_count * sizeof(vren::directional_light) : 1
	};
	descriptor_set_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptor_set,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);

	// Spot lights
	buffer_info = {
		.buffer = m_spot_light_buffer.m_buffer.m_handle,
		.offset = 0,
		.range = m_spot_light_count > 0 ? m_spot_light_count * sizeof(vren::spot_light) : 1
	};
	descriptor_set_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptor_set,
		.dstBinding = 2,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}
