#pragma once

#include <type_traits>
#include <memory>
#include <array>

#include "config.hpp"
#include "simple_draw.hpp"
#include "render_list.hpp"
#include "gpu_allocator.hpp"

#include <glm/glm.hpp>

namespace vren
{
	void get_supported_layers(std::vector<VkLayerProperties>& layers);
	bool does_support_layers(std::vector<char const*> const& layers);

	struct renderer_info
	{
		char const* m_app_name;
		uint32_t m_app_version;
		std::vector<char const*> m_layers;
		std::vector<char const*> m_extensions;
		std::vector<char const*> m_device_extensions;
	};

	struct camera
	{
		glm::mat4 m_view_matrix;
		glm::mat4 m_projection_matrix;
	};

	struct renderer
	{
	public:
		struct queue_families {
			uint32_t m_graphics_idx = -1;
			uint32_t m_compute_idx = -1;
			uint32_t m_transfer_idx = -1;

			bool is_fine() const {
				return m_graphics_idx >= 0 && m_compute_idx >= 0 && m_transfer_idx >= 0;
			}
		};

	private:
		VkInstance create_instance();
		VkDebugUtilsMessengerEXT setup_debug_messenger();
		vren::renderer::queue_families get_queue_families(VkPhysicalDevice physical_device);
		VkPhysicalDevice find_physical_device();
		VkDevice create_logical_device();
		std::vector<VkQueue> get_queues();
		VkRenderPass create_render_pass();

		void create_sync_objects();

	public:
		vren::renderer_info m_info;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_physical_device;
		vren::renderer::queue_families m_queue_families;
		VkDevice m_device;
		std::vector<VkQueue> m_queues;
		VkRenderPass m_render_pass;

		vren::gpu_allocator m_gpu_allocator;

		VkClearColorValue m_clear_color = {1.0f, 0.0f, 0.0f, 1.0f};
		static const VkFormat m_color_output_format = VK_FORMAT_B8G8R8A8_SRGB;

		// Descriptor pools
		VkDescriptorPool m_descriptor_pool;

		/* Command pools */
		VkCommandPool m_graphics_command_pool;
		VkCommandPool m_transfer_command_pool;

		/* Subpasses */
		vren::simple_draw_pass m_simple_draw_pass;

		/* Per frame data */
		std::array<VkSemaphore, VREN_MAX_FRAME_COUNT> m_render_finished_semaphores;
		std::array<VkCommandBuffer, VREN_MAX_FRAME_COUNT> m_graphics_command_buffers;

		void create_descriptor_pools();
		void create_command_pools();
		void alloc_command_buffers();

		renderer(renderer_info& info);
		~renderer();

		struct target
		{
			VkFramebuffer m_framebuffer;
			VkRect2D m_render_area;
			VkViewport m_viewport;
			VkRect2D m_scissor;
		};

		VkSemaphore render(
			uint32_t frame_idx,
			vren::renderer::target const& target,
			vren::render_list const& render_list,
			vren::camera const& camera,
			std::vector<VkSemaphore> const& wait_semaphores = {},
			VkFence signal_fence = VK_NULL_HANDLE
		);
	};
}

