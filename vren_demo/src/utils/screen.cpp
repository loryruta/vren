#include "screen.hpp"

#include "descriptor_pool.hpp"
#include "utils/image.hpp"

vren_demo::screen::screen(
	std::shared_ptr<vren::imgui_renderer> const& imgui_renderer,
	VkRenderPass render_pass
) :
	m_imgui_renderer(imgui_renderer),
	m_render_pass(render_pass)
{}

vren_demo::screen::~screen()
{
}

void vren_demo::screen::_recreate_framebuffer(ImVec2 const& size)
{
	m_color_buffer = std::make_shared<color_buffer>(
		vren::vk_utils::create_
	);

	m_framebuffer = std::make_shared<vren::vk_framebuffer>(
		vren::create_framebuffer(
			m_imgui_renderer->m_context,
			m_render_pass,
			{  },
			size
		)
	);
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




