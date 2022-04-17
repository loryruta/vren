#pragma once

#include "base/resource_container.hpp"
#include "vk_helpers/vk_raii.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"
#include "renderer.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Debug renderer
	// ------------------------------------------------------------------------------------------------

	class debug_renderer
	{
	public:
		static constexpr float k_point_size = 0.01f;

		struct point
		{
			glm::vec3 m_position;
			glm::vec3 m_color;
		};

		struct line
		{
			glm::vec3 m_from, m_to;
			glm::vec3 m_color;
		};

		struct vertex
		{
			glm::vec3 m_position;
			glm::vec3 m_color;
		};

		struct instance_data
		{
			glm::mat4 m_transform;
			glm::vec3 m_color;
		};

		struct push_constants
		{
			glm::mat4 m_camera_view;
			glm::mat4 m_camera_projection;
		};

	private:
		vren::context const* m_context;

	public:
		vren::vk_render_pass m_render_pass;

	private:
		vren::vk_utils::buffer m_identity_instance_buffer;

		vren::vk_utils::resizable_buffer m_lines_vertex_buffer;
		size_t m_lines_vertex_count = 0;

		vren::vk_utils::pipeline m_pipeline;

		vren::vk_utils::buffer create_identity_instance_buffer();

		vren::vk_render_pass create_render_pass();
		vren::vk_utils::pipeline create_graphics_pipeline();

	public:
		debug_renderer(vren::context const& context);

		void clear();

		void draw_point(vren::debug_renderer::point point);
		void draw_line(vren::debug_renderer::line line);

		void render(
			uint32_t frame_idx,
			VkCommandBuffer command_buffer,
			vren::resource_container& resource_container,
			vren::render_target const& render_target,
			vren::debug_renderer::push_constants const& push_constants
		);
	};
}
