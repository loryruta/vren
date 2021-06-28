
#include "render_list.hpp"

#include "renderer.hpp"

vren::render_list::render_list(vren::renderer& renderer) :
	m_renderer(renderer)
{}


vren::render_object& vren::render_list::create_render_object()
{
	size_t pos = m_render_objects.size();
	m_position_by_idx.emplace_back(pos);

	auto& inserted_obj = m_render_objects.emplace_back(&m_renderer);
	inserted_obj.m_render_list_idx = m_next_idx;
	m_next_idx++;

	return inserted_obj;
}

void vren::render_list::destroy_render_object(uint32_t idx)
{
	// The render_object to delete is deleted and is brought to the last position.
	// After that, the position by index is updated, and the last element is popped back.

	auto& cur_obj = m_render_objects.at(m_position_by_idx.at(idx));
	auto& last_obj = m_render_objects.back();

	cur_obj.~render_object();

	vren::render_object tmp_obj = cur_obj; // The deleted element is swapped with the last element.
	cur_obj = last_obj;
	last_obj = tmp_obj;

	m_position_by_idx[last_obj.m_render_list_idx] = m_position_by_idx.at(idx);
	m_position_by_idx[cur_obj.m_render_list_idx] = UINT32_MAX; // The position of the deleted object is set to an invalid value.

	m_render_objects.pop_back();
}

vren::render_object& vren::render_list::get_render_object(uint32_t idx)
{
	return m_render_objects.at(m_position_by_idx.at(idx));
}
