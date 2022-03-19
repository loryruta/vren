#include "screen.hpp"

#include "pooling/descriptor_pool.hpp"
#include "utils/image.hpp"

vren_demo::screen::screen(
	std::shared_ptr<vren::context> const& ctx,
	std::shared_ptr<vren::vk_render_pass> const& render_pass,
	ImVec2 const& init_size
) :
	m_context(ctx),
	m_framebuffer(vren::vk_utils::custom_framebuffer(ctx, render_pass, init_size.x, init_size.y))
{}

vren_demo::screen::~screen()
{}

void vren_demo::screen::_recreate_framebuffer(ImVec2 const& size)
{
}

void vren_demo::screen::show(
	ImVec2 const& size,
	std::function<void(vren::render_target const&)> const& render_func
)
{
	if (size.x <= 0 || size.y <= 0) {
		return;
	}

	if (!m_current_size.has_value() || size.x != m_current_size->x || size.y != m_current_size->y)
	{
		_recreate_framebuffer(size);

		m_current_size = size;
	}
}




