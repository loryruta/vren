#include "render_object.hpp"

#include "renderer.hpp"

#include <iostream>


vren::render_object::render_object(vren::renderer* renderer) :
	m_renderer(renderer)
{
	m_vertices_count  = 0;
	m_indices_count   = 0;
	m_instances_count = 0;

	m_material = nullptr;
}

vren::render_object::render_object(vren::render_object&& other) noexcept
{
	*this = std::move(other);
}

vren::render_object::~render_object()
{
	m_renderer->m_gpu_allocator->destroy_buffer_if_any(m_vertex_buffer);
	m_renderer->m_gpu_allocator->destroy_buffer_if_any(m_indices_buffer);
	m_renderer->m_gpu_allocator->destroy_buffer_if_any(m_instances_buffer);
}

vren::render_object& vren::render_object::operator=(vren::render_object&& other) noexcept
{
	m_renderer = other.m_renderer;

	m_vertex_buffer  = std::move(other.m_vertex_buffer);
	m_vertices_count = other.m_vertices_count;

	m_indices_buffer = std::move(other.m_indices_buffer);
	m_indices_count  = other.m_indices_count;

	m_instances_buffer = std::move(other.m_instances_buffer);
	m_instances_count  = other.m_instances_count;

	m_material = other.m_material;

	return *this;
}

bool vren::render_object::is_valid() const
{
	return
		m_vertex_buffer.is_valid() &&
		m_indices_buffer.is_valid() &&
		m_instances_buffer.is_valid() &&
		m_material;
}

void vren::render_object::set_vertices_data(vren::vertex const* vertices_data, size_t vertices_count)
{
	auto& allocator = m_renderer->m_gpu_allocator;

	allocator->destroy_buffer_if_any(m_vertex_buffer);

	if (vertices_count > 0)
	{
		allocator->alloc_device_only_buffer(m_vertex_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vren::vertex) * vertices_count);
		allocator->update_buffer(m_vertex_buffer, vertices_data, sizeof(vren::vertex) * vertices_count, 0);
	}

	m_vertices_count = vertices_count;
}

void vren::render_object::set_indices_data(uint32_t const* indices_data, size_t indices_count)
{
	auto& allocator = m_renderer->m_gpu_allocator;

	allocator->destroy_buffer_if_any(m_indices_buffer);

	if (indices_count > 0)
	{
		allocator->alloc_device_only_buffer(m_indices_buffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(uint32_t) * indices_count);
		allocator->update_buffer(m_indices_buffer, indices_data, sizeof(uint32_t) * indices_count, 0);
	}

	m_indices_count = indices_count;
}

void vren::render_object::set_instances_data(vren::instance_data const* instances_data, size_t instances_count)
{
	auto& allocator = m_renderer->m_gpu_allocator;

	allocator->destroy_buffer_if_any(m_instances_buffer);

	if (instances_count > 0)
	{
		allocator->alloc_device_only_buffer(m_instances_buffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(vren::instance_data) * instances_count);
		allocator->update_buffer(m_instances_buffer, instances_data, sizeof(vren::instance_data) * instances_count, 0);
	}

	m_instances_count = instances_count;
}

