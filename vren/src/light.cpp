#include "light.hpp"

#include "context.hpp"

vren::lights_array::lights_array(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{
	m_point_lights_ssbo_alloc_size = 0;
	m_directional_lights_ssbo_alloc_size = 0;
}

template<typename _light_type>
void vren::lights_array::_try_realloc_light_buffer(std::shared_ptr<vren::vk_utils::buffer>& buf, size_t& buf_len, size_t el_count)
{
	auto el_num = (size_t) glm::max<size_t>((size_t) glm::ceil((float) el_count / (float) 256) * 256, 256);
	size_t alloc_size = sizeof(glm::vec4) + el_num * sizeof(_light_type);

	if (buf_len < alloc_size)
	{
		buf = std::make_shared<vren::vk_utils::buffer>(
			vren::vk_utils::alloc_host_visible_buffer(m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, alloc_size)
		);

		buf_len = alloc_size;
	}
}

void vren::lights_array::_try_realloc_light_buffers()
{
	_try_realloc_light_buffer<vren::point_light>(
		m_point_lights_ssbo,
		m_point_lights_ssbo_alloc_size,
		m_point_lights.size()
	);

	_try_realloc_light_buffer<vren::directional_light>(
		m_directional_lights_ssbo,
		m_directional_lights_ssbo_alloc_size,
		m_directional_lights.size()
	);
}

std::pair<std::reference_wrapper<vren::point_light>, uint32_t> vren::lights_array::create_point_light()
{
	return std::make_pair(
		std::ref(m_point_lights.emplace_back()),
		m_point_lights.size() - 1
	);
}

std::pair<std::reference_wrapper<vren::directional_light>, uint32_t> vren::lights_array::create_directional_light()
{
	return std::make_pair(
		std::ref(m_directional_lights.emplace_back()),
		m_point_lights.size() - 1
	);
}

vren::point_light& vren::lights_array::get_point_light(uint32_t idx)
{
	return m_point_lights.at(idx);
}

vren::directional_light& vren::lights_array::get_directional_light(uint32_t idx)
{
	return m_directional_lights.at(idx);
}

void vren::lights_array::update_device_buffers()
{
	_try_realloc_light_buffers();

	if (m_point_lights_ssbo_alloc_size > 0) // Point lights
	{
		auto num = (uint32_t) m_point_lights.size();

		vren::vk_utils::update_host_visible_buffer(
			*m_context,
			*m_point_lights_ssbo,
			&num,
			sizeof(uint32_t),
			0
		);

		vren::vk_utils::update_host_visible_buffer(
			*m_context,
			*m_point_lights_ssbo,
			m_point_lights.data(),
			m_point_lights.size() * sizeof(vren::point_light),
			sizeof(glm::vec4)
		);
	}

	if (m_directional_lights_ssbo_alloc_size > 0) // Directional lights
	{
		auto num = (uint32_t) m_directional_lights.size();

		vren::vk_utils::update_host_visible_buffer(
			*m_context,
			*m_directional_lights_ssbo,
			&num,
			sizeof(uint32_t),
			0
		);

		vren::vk_utils::update_host_visible_buffer(
			*m_context,
			*m_directional_lights_ssbo,
			m_directional_lights.data(),
			m_directional_lights.size() * sizeof(vren::directional_light),
			sizeof(glm::vec4)
		);
	}
}

void vren::lights_array::update_descriptor_set(vren::frame& frame, VkDescriptorSet descriptor_set) const
{
	std::vector<VkDescriptorBufferInfo> buffers_info{};

	{ // Point lights
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = m_point_lights_ssbo->m_buffer->m_handle;
		buffer_info.offset = 0;
		buffer_info.range = m_point_lights_ssbo_alloc_size;

		buffers_info.push_back(buffer_info);
	}

	{ // Directional lights
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = m_directional_lights_ssbo->m_buffer->m_handle;
		buffer_info.offset = 0;
		buffer_info.range  = m_directional_lights_ssbo_alloc_size;

		buffers_info.push_back(buffer_info);
	}

	VkWriteDescriptorSet desc_set_write{};
	desc_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc_set_write.dstSet = descriptor_set;
	desc_set_write.dstBinding = VREN_POINT_LIGHTS_BINDING; // VREN_DIRECTIONAL_LIGHTS_BINDING
	desc_set_write.dstArrayElement = 0;
	desc_set_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc_set_write.descriptorCount = buffers_info.size();
	desc_set_write.pBufferInfo = buffers_info.data();

	vkUpdateDescriptorSets(m_context->m_device, 1, &desc_set_write, 0, nullptr);

	frame.track_resource(m_point_lights_ssbo);
	frame.track_resource(m_directional_lights_ssbo);
}
