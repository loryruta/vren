#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "config.hpp"
#include "gpu_allocator.hpp"

namespace vren
{
	// Forward decl
	class renderer;

	struct point_light
	{
		glm::vec3 m_position;
		float _pad;

		glm::vec3 m_color;
		float _pad_1;
	};

	struct directional_light
	{
		glm::vec3 m_direction;
		float _pad;

		glm::vec3 m_color;
		float _pad_1;
	};

	class lights_array
	{
	private:
		std::shared_ptr<vren::renderer> m_renderer;

		std::vector<point_light> m_point_lights;
		std::vector<directional_light> m_directional_lights;

		std::shared_ptr<vren::vk_utils::buffer> m_point_lights_ssbo;
		size_t m_point_lights_ssbo_alloc_size;

		std::shared_ptr<vren::vk_utils::buffer> m_directional_lights_ssbo;
		size_t m_directional_lights_ssbo_alloc_size;

		template<typename _light_type>
		void _try_realloc_light_buffer(std::shared_ptr<vren::vk_utils::buffer>& buf, size_t& buf_len, size_t el_count);
		void _try_realloc_light_buffers();

	public:
		explicit lights_array(std::shared_ptr<vren::renderer> const& renderer);

		// todo index in point light like render_object
		// todo index -> id
		std::pair<std::reference_wrapper<vren::point_light>, uint32_t> create_point_light();
		std::pair<std::reference_wrapper<vren::directional_light>, uint32_t> create_directional_light();

		vren::point_light& get_point_light(uint32_t idx);
		vren::directional_light& get_directional_light(uint32_t idx);

		void update_device_buffers();
		void update_descriptor_set(VkDescriptorSet descriptor_set) const;
	};
}
