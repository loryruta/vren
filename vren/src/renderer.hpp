#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "config.hpp"
#include "simple_draw.hpp"
#include "render_list.hpp"
#include "light_array.hpp"
#include "pools/command_pool.hpp"
#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------

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

	// ------------------------------------------------------------------------------------------------
	// Renderer
	// ------------------------------------------------------------------------------------------------

	class renderer
	{
	public:
		static constexpr size_t k_max_point_lights_count = VREN_POINT_LIGHTS_BUFFER_SIZE / sizeof(vren::point_light);
		static constexpr size_t k_max_directional_lights_count = VREN_DIRECTIONAL_LIGHTS_BUFFER_SIZE / sizeof(vren::directional_light);
		static constexpr size_t k_max_spot_lights_count = VREN_SPOT_LIGHTS_BUFFER_SIZE / sizeof(vren::spot_light);

		std::shared_ptr<vren::vk_render_pass> m_render_pass;

		VkClearColorValue m_clear_color = {1.0f, 0.0f, 0.0f, 1.0f};

		vren::simple_draw_pass m_simple_draw_pass;

        std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> m_point_lights_buffers;
        std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> m_directional_lights_buffers;
		std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> m_spot_lights_buffers;

	private:
		vren::context const* m_context;

        vren::vk_render_pass create_render_pass();

        std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> create_point_lights_buffers();
        std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> create_directional_lights_buffers();
        std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT> create_spot_lights_buffers();

		void upload_light_array(int frame_idx, vren::light_array const& lights_arr);

	public:
		explicit renderer(vren::context const& ctx);
		~renderer();

		void write_light_array_descriptor_set(uint32_t frame_idx, VkDescriptorSet desc_set);

		void render(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			vren::render_target const& target,
			vren::render_list const& render_list,
			vren::light_array const& lights_arr,
			vren::camera const& camera
		);
	};
}
