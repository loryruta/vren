#include "render_list.hpp"

#include "render_object.hpp"

vren::render_list::render_list(std::shared_ptr<vren::context> const& ctx) :
	m_context(ctx)
{}

vren::render_object& vren::render_list::create_render_object()
{
	size_t pos = m_render_objects.size();
	m_position_by_idx.emplace_back(pos);

	auto& render_obj = m_render_objects.emplace_back(shared_from_this());
	render_obj.m_idx = m_next_idx;
	m_next_idx++;

	return render_obj;
}

vren::render_object& vren::render_list::get_render_object(uint32_t idx)
{
	return m_render_objects.at(m_position_by_idx.at(idx));
}

void vren::render_list::delete_render_object(uint32_t idx)
{
	uint32_t pos = m_position_by_idx.at(idx);

	auto& cur_obj  = m_render_objects.at(pos);
	auto& last_obj = m_render_objects.back();

	cur_obj.~render_object();

	vren::render_object tmp_obj = std::move(cur_obj);
	cur_obj  = std::move(last_obj);
	last_obj = std::move(tmp_obj);

	m_position_by_idx[last_obj.m_idx] = m_position_by_idx.at(idx);
	m_position_by_idx[cur_obj.m_idx]  = UINT32_MAX;

	cur_obj.m_idx = UINT32_MAX;

	m_render_objects.pop_back();
}

std::shared_ptr<vren::render_list> vren::render_list::create(std::shared_ptr<vren::context> const& ctx)
{
	return std::shared_ptr<vren::render_list>(new vren::render_list(ctx));
}
