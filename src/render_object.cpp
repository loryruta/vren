#include "render_object.hpp"

#include "renderer.hpp"

vren::render_object::render_object(vren::renderer* renderer) :
	m_renderer(renderer)
{}

vren::render_object::~render_object()
{
	for (auto& vertex_buf : m_vertex_buffers) {
		m_renderer->m_gpu_allocator.destroy_buffer_if_any(vertex_buf);
	}

	m_renderer->m_gpu_allocator.destroy_buffer_if_any(m_indices_buffer);

	for (auto& instance_buf : m_instances_buffers) {
		m_renderer->m_gpu_allocator.destroy_buffer_if_any(instance_buf);
	}
}

void vren::render_object::set_vertex_data(vren::vertex const* vertices_data, size_t count)
{
	vren::gpu_allocator& allocator = m_renderer->m_gpu_allocator;

	for (auto& vbo : m_vertex_buffers) {
		allocator.destroy_buffer_if_any(vbo);
	}

	m_vertex_buffers.resize(1);

	allocator.alloc_device_only_buffer(m_vertex_buffers.at(0), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vren::vertex) * count);
	allocator.update_buffer(m_vertex_buffers.at(0), vertices_data, sizeof(vren::vertex) * count, 0);
}

void vren::render_object::set_indices_data(uint32_t const* indices_data, size_t count)
{
	vren::gpu_allocator& allocator = m_renderer->m_gpu_allocator;

	allocator.destroy_buffer_if_any(m_indices_buffer);

	allocator.alloc_device_only_buffer(m_indices_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(uint32_t) * count);
	allocator.update_buffer(m_indices_buffer, indices_data, sizeof(uint32_t) * count, 0);

	m_indices_count = count;
}

void vren::render_object::set_instances_data(vren::instance_data const* instances_data, size_t count)
{
	vren::gpu_allocator& allocator = m_renderer->m_gpu_allocator;

	for (auto& idb : m_instances_buffers) {
		allocator.destroy_buffer_if_any(idb);
	}

	m_instances_buffers.resize(1);

	allocator.alloc_device_only_buffer(m_instances_buffers.at(0), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vren::instance_data) * count);
	allocator.update_buffer(m_instances_buffers.at(0), instances_data, sizeof(vren::instance_data) * count, 0);

	m_instances_count = count;
}

