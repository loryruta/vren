#include "render_object.hpp"

#include <iostream>

vren::render_object::render_object(render_list& render_list) :
	m_render_list(&render_list)
{
	m_vertices_count = 0;
	m_indices_count = 0;
	m_instances_count = 0;
}

vren::render_object::render_object(vren::render_object&& other) noexcept
{
	*this = std::move(other);
}

vren::render_object& vren::render_object::operator=(vren::render_object&& other) noexcept
{
	m_render_list = other.m_render_list;

	m_vertices_buffer = std::move(other.m_vertices_buffer);
	m_vertices_count = other.m_vertices_count;

	m_indices_buffer = std::move(other.m_indices_buffer);
	m_indices_count = other.m_indices_count;

	m_instances_buffer = std::move(other.m_instances_buffer);
	m_instances_count = other.m_instances_count;

	m_material = other.m_material;

	return *this;
}

bool vren::render_object::is_valid() const
{
	return m_vertices_buffer && m_indices_buffer && m_instances_buffer;
}

void vren::render_object::set_vertices_buffer(std::shared_ptr<vren::vk_utils::buffer> const& vertices_buf, size_t vertices_count)
{
	m_vertices_buffer = vertices_buf;
	m_vertices_count  = vertices_count;
}

void vren::render_object::set_indices_buffer(std::shared_ptr<vren::vk_utils::buffer> const& indices_buf, size_t indices_count)
{
	m_indices_buffer = indices_buf;
	m_indices_count  = indices_count;
}

void vren::render_object::set_instances_buffer(std::shared_ptr<vren::vk_utils::buffer> const& instances_buf, size_t instances_count)
{
	m_instances_buffer = instances_buf;
	m_instances_count  = instances_count;
}
