#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "context.hpp"
#include "descriptor_pool.hpp"
#include "render_list.hpp"
#include "light_array.hpp"
#include "command_pool.hpp"
#include "resource_container.hpp"

namespace vren
{
	// Forward decl
	class simple_draw_pass;

	struct camera
	{
		glm::vec3 m_position;
		float _pad;
		glm::mat4 m_view;
		glm::mat4 m_projection;
	};

	struct render_target
	{
		VkFramebuffer m_framebuffer;
		VkRect2D m_render_area;
		VkViewport m_viewport;
		VkRect2D m_scissor;
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// renderer
	// --------------------------------------------------------------------------------------------------------------------------------

	class renderer : public std::enable_shared_from_this<renderer>
	{
	public:
		static VkFormat const k_color_output_format = VK_FORMAT_B8G8R8A8_UNORM;

		std::shared_ptr<context> m_context;

		vren::vk_render_pass m_render_pass;

		vren::vk_descriptor_set_layout m_material_descriptor_set_layout;
		vren::vk_descriptor_set_layout m_light_array_descriptor_set_layout;

		std::shared_ptr<vren::descriptor_pool> m_material_descriptor_pool;
		std::shared_ptr<vren::descriptor_pool> m_light_array_descriptor_pool;

		//vren::vk_descriptor_set_layout m_material_descriptor_set_layout;
		//std::shared_ptr<vren::descriptor_set_pool> m_lights_descriptor_set_pool;

		VkClearColorValue m_clear_color = {1.0f, 0.0f, 0.0f, 1.0f};

		std::unique_ptr<vren::simple_draw_pass> m_simple_draw_pass;

		/* Light buffers */
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> m_point_lights_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> m_directional_lights_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> m_spot_lights_buffers;

	private:
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> _create_point_lights_buffers();
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> _create_directional_lights_buffer();
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> _create_spot_lights_buffers();

		renderer(std::shared_ptr<context> const& ctx);

		void _init();

		static vren::vk_render_pass _create_render_pass(
			std::shared_ptr<vren::context> const& ctx
		);

	public:
		~renderer();

		void render(
			uint32_t frame_idx,
			vren::resource_container& resource_container,
			vren::render_target const& target,
			VkSemaphore src_semaphore,
			VkSemaphore dst_semaphore,
			vren::render_list const& render_list,
			vren::light_array const& lights_array,
			vren::camera const& camera
		);

		static std::shared_ptr<vren::renderer> create(std::shared_ptr<context> const& ctx);
	};
}
