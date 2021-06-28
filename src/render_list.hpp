
#pragma once

#include "render_object.hpp"

#include <vector>
#include <glm/glm.hpp>

namespace vren
{
	// Forward decl
	class renderer;

	struct render_list
	{
	private:
		vren::renderer& m_renderer;

		std::vector<uint32_t> m_position_by_idx;
		uint32_t m_next_idx = 0;

		std::vector<vren::render_object> m_render_objects;

	public:
		render_list(vren::renderer& renderer);
		~render_list() = default;

		vren::render_object& create_render_object();
		vren::render_object& get_render_object(uint32_t idx);
		void destroy_render_object(uint32_t idx);

		[[nodiscard]] auto begin() const -> decltype(m_render_objects.begin()) {
			return m_render_objects.begin();
		}

		[[nodiscard]] auto end() const -> decltype(m_render_objects.end()) {
			return m_render_objects.end();
		}
	};
}
