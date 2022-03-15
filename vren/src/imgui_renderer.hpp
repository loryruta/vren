#pragma once

#include <memory>
#include <functional>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "renderer.hpp"
#include "command_pool.hpp"
#include "descriptor_pool.hpp"

namespace vren
{
	// --------------------------------------------------------------------------------------------------------------------------------
	// imgui_descriptor_pool
	// --------------------------------------------------------------------------------------------------------------------------------

	class imgui_descriptor_pool : public vren::descriptor_pool
	{
	public:
		imgui_descriptor_pool(std::shared_ptr<vren::context> const& ctx);
		~imgui_descriptor_pool();

		VkDescriptorPool create_descriptor_pool(int max_sets) override;

		vren::vk_descriptor_set acquire_descriptor_set();

		void write_descriptor_set(
			vren::vk_descriptor_set const& desc_set,
			VkSampler sampler,
			VkImageView image_view,
			VkImageLayout image_layout
		);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// imgui_renderer
	// --------------------------------------------------------------------------------------------------------------------------------

	class imgui_renderer
	{
	private:
		VkDescriptorPool m_internal_descriptor_pool;
		VkRenderPass m_render_pass;

		void _init_render_pass();

	public:
		std::shared_ptr<vren::context> m_context;
		GLFWwindow* m_window;
		std::shared_ptr<vren::imgui_descriptor_pool> m_descriptor_pool;

		explicit imgui_renderer(std::shared_ptr<vren::context> const& context, GLFWwindow* window);
		~imgui_renderer();

		void record_commands(
            vren::resource_container& res_container,
			vren::vk_command_buffer const& cmd_buf,
			vren::render_target const& target,
			std::function<void()> const& show_guis_func
		);
	};
}
