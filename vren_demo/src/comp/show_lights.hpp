#pragma once

#include "resource_container.hpp"
#include "renderer.hpp"
#include "utils/vk_toolbox.hpp"

namespace vren_demo
{
	class show_lights
	{
	public:
		struct push_constants
		{
			float m_visualizer_scale;
		};

	private:
		vren::renderer* m_renderer;

		std::shared_ptr<vren::vk_utils::self_described_shader> m_shader;
		vren::vk_utils::self_described_compute_pipeline m_pipeline;

	public:
		show_lights(vren::renderer& renderer);

		void write_instance_buffer_descriptor_set(VkDescriptorSet desc_set, VkBuffer instance_buf);

		void dispatch(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			vren::light_array const& light_arr,
			vren::render_object& point_light_visualizer,
			push_constants push_constants
		);
	};
}
