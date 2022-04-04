#pragma once

#include <vren/base/resource_container.hpp>
#include <vren/renderer.hpp>
#include <vren/toolbox.hpp>

namespace vren_demo
{
	class show_lights
	{
	public:
		static constexpr uint32_t k_light_array_descriptor_set_idx = 0;
		static constexpr uint32_t k_light_array_movement_buffer_descriptor_set_idx = 1;

		struct push_constants
		{
			float m_visualizer_scale;
		};

	private:
		vren::context const* m_context;
		vren::renderer* m_renderer;

		vren::vk_utils::pipeline m_pipeline;

	public:
		show_lights(vren::context const& ctx, vren::renderer& renderer);

		void write_instance_buffer_descriptor_set(VkDescriptorSet desc_set, VkBuffer instance_buf);

		void dispatch(
			int frame_idx,
			VkCommandBuffer cmd_buf,
			vren::resource_container& res_container,
			vren::light_array const& light_arr,
			vren::render_object& point_light_visualizer,
			show_lights::push_constants push_constants
		);
	};
}
