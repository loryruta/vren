#include "light.hpp"

#include "renderer.hpp"

vren::lights_array::lights_array(vren::renderer& renderer) :
	m_renderer(renderer)
{
	m_point_lights_ssbo_alloc_size       = 0;
	m_directional_lights_ssbo_alloc_size = 0;
}

vren::lights_array::~lights_array()
{
	m_renderer.m_gpu_allocator->destroy_buffer_if_any(m_point_lights_ssbo);
	m_renderer.m_gpu_allocator->destroy_buffer_if_any(m_directional_lights_ssbo);
}

void vren::lights_array::_adapt_gpu_buffers_size()
{
	{ // Point lights
		size_t req_alloc_size = ((size_t) glm::ceil((float) m_point_lights.size() / (float) 256) * 256) * sizeof(vren::point_light);
		if (req_alloc_size != m_point_lights_ssbo_alloc_size)
		{
			m_renderer.m_gpu_allocator->destroy_buffer_if_any(
				m_point_lights_ssbo
			);
			m_point_lights_ssbo_alloc_size = req_alloc_size;

			if (req_alloc_size > 0)
			{
				m_renderer.m_gpu_allocator->alloc_host_visible_buffer(
					m_point_lights_ssbo,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					sizeof(vren::point_light) * req_alloc_size
				);
			}
		}
	}

	{ // Directional lights
		size_t req_alloc_size = ((size_t) glm::ceil((float) m_directional_lights.size() / (float) 256) * 256) * sizeof(vren::directional_light);
		if (req_alloc_size != m_directional_lights_ssbo_alloc_size)
		{
			m_renderer.m_gpu_allocator->destroy_buffer_if_any(
				m_directional_lights_ssbo
			);
			m_directional_lights_ssbo_alloc_size = req_alloc_size;

			if (req_alloc_size > 0)
			{
				m_renderer.m_gpu_allocator->alloc_host_visible_buffer(
					m_directional_lights_ssbo,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					sizeof(vren::point_light) * req_alloc_size
				);
			}
		}
	}
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
	_adapt_gpu_buffers_size();

	if (m_point_lights_ssbo_alloc_size > 0) // Point lights
	{
		m_renderer.m_gpu_allocator->update_host_visible_buffer(
			m_point_lights_ssbo,
			m_point_lights.data(),
			m_point_lights.size() * sizeof(vren::point_light),
			0
		);
	}

	if (m_directional_lights_ssbo_alloc_size > 0) // Directional lights
	{
		m_renderer.m_gpu_allocator->update_host_visible_buffer(
			m_directional_lights_ssbo,
			m_directional_lights.data(),
			m_directional_lights.size() * sizeof(vren::directional_light),
			0
		);
	}
}

void vren::lights_array::update_descriptor_set(VkDescriptorSet descriptor_set) const
{
	std::vector<VkDescriptorBufferInfo> buffers_info{};

	{ // Point lights
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = m_point_lights_ssbo.m_buffer;
		buffer_info.offset = 0;
		buffer_info.range = m_point_lights_ssbo_alloc_size;

		buffers_info.push_back(buffer_info);
	}

	{ // Directional lights
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = m_directional_lights_ssbo.m_buffer;
		buffer_info.offset = 0;
		buffer_info.range  = m_directional_lights_ssbo_alloc_size;

		buffers_info.push_back(buffer_info);
	}

	VkWriteDescriptorSet descriptor_set_write{};
	descriptor_set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_set_write.dstSet = descriptor_set;
	descriptor_set_write.dstBinding = VREN_POINT_LIGHTS_BINDING; // VREN_DIRECTIONAL_LIGHTS_BINDING
	descriptor_set_write.dstArrayElement = 0;
	descriptor_set_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_set_write.descriptorCount = buffers_info.size();
	descriptor_set_write.pBufferInfo = buffers_info.data();

	vkUpdateDescriptorSets(m_renderer.m_device, 1, &descriptor_set_write, 0, nullptr);
}
