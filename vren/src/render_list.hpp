#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "render_object.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// render_list
	// ------------------------------------------------------------------------------------------------

	class render_list
	{
	public:
		std::vector<uint32_t> m_position_by_idx;
		uint32_t m_next_idx = 0;

		std::vector<vren::render_object> m_render_objects;

	public:
		explicit render_list() = default;
		~render_list() = default;

		vren::render_object& create_render_object();
		vren::render_object& get_render_object(uint32_t idx);
		void delete_render_object(uint32_t idx);

		auto begin() const -> decltype(m_render_objects.begin()) {
			return m_render_objects.begin();
		}

		auto end() const -> decltype(m_render_objects.end()) {
			return m_render_objects.end();
		}

		void clear()
		{
			m_position_by_idx.clear();
			m_render_objects.clear();
		}
	};
}
