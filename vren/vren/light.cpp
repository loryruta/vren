#include "light.hpp"

#include "base/base.hpp"
#include "context.hpp"
#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::light_array::light_array(vren::context const& context) :
	m_context(&context),
	m_point_light_position_buffer([&]()
	{
		vren::vk_utils::buffer buffer = vren::vk_utils::alloc_host_visible_buffer(
			*m_context,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VREN_MAX_POINT_LIGHT_COUNT * sizeof(glm::vec4),
			true
		);
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, "point_light_position_buffer");
		return buffer;
	}()),
	m_point_light_buffer([&]()
	{
		vren::vk_utils::buffer buffer = vren::vk_utils::alloc_host_visible_buffer(
			*m_context,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VREN_MAX_POINT_LIGHT_COUNT * sizeof(vren::point_light),
			true
		);
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, "point_light_buffer");
		return buffer;
	}()),
	m_point_light_count(0),
	m_directional_light_buffer([&]()
	{
		vren::vk_utils::buffer buffer = vren::vk_utils::alloc_host_visible_buffer(
			*m_context,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VREN_MAX_DIRECTIONAL_LIGHT_COUNT * sizeof(vren::directional_light),
			true
		);
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, "directional_light_buffer");
		return buffer;
	}()),
	m_directional_light_count(0)
{
}

void vren::light_array::write_descriptor_set(VkDescriptorSet descriptor_set) const
{
	vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 0, m_point_light_position_buffer.m_buffer.m_handle, m_point_light_count * sizeof(glm::vec4), 0);
	vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 1, m_point_light_buffer.m_buffer.m_handle, m_point_light_count * sizeof(vren::point_light), 0);

	vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 2, m_directional_light_buffer.m_buffer.m_handle, m_directional_light_count * sizeof(vren::directional_light), 0);
}
