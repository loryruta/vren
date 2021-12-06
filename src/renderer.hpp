#pragma once

#include <type_traits>
#include <memory>
#include <array>
#include <filesystem>

#include "config.hpp"
#include "simple_draw.hpp"
#include "render_list.hpp"
#include "gpu_allocator.hpp"
#include "frame.hpp"
#include "material.hpp"

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
		glm::mat4 m_view;
		glm::mat4 m_projection;
	};

	struct renderer
	{
	public:
		struct queue_families {
			uint32_t m_graphics_idx = -1;
			uint32_t m_compute_idx = -1;
			uint32_t m_transfer_idx = -1;

			[[nodiscard]] bool is_fine() const {
				return m_graphics_idx >= 0 && m_compute_idx >= 0 && m_transfer_idx >= 0;
			}
		};

	private:
		VkInstance create_instance();
		VkDebugUtilsMessengerEXT setup_debug_messenger();
		vren::renderer::queue_families get_queue_families(VkPhysicalDevice physical_device);
		VkPhysicalDevice find_physical_device();
		VkDevice create_logical_device();
		VmaAllocator create_allocator();
		std::vector<VkQueue> get_queues();
		VkRenderPass create_render_pass();

		void create_white_texture();

	public:
		vren::renderer_info m_info;

		VkInstance m_instance;
		VkDebugUtilsMessengerEXT m_debug_messenger;
		VkPhysicalDevice m_physical_device;
		vren::renderer::queue_families m_queue_families;
		VkDevice m_device;
		std::vector<VkQueue> m_queues;
		VkQueue m_transfer_queue;
		VkQueue m_graphics_queue;
		VkQueue m_compute_queue;
		VkRenderPass m_render_pass;

		VmaAllocator m_allocator; // todo initialize it
		vren::gpu_allocator m_gpu_allocator;

		VkClearColorValue m_clear_color = {1.0f, 0.0f, 0.0f, 1.0f};
		static const VkFormat m_color_output_format = VK_FORMAT_B8G8R8A8_SRGB;

		/* Command pools */
		VkCommandPool m_graphics_command_pool;
		VkCommandPool m_transfer_command_pool;

		/* Subpasses */
		vren::simple_draw_pass m_simple_draw_pass;

		vren::material_descriptor_set_pool m_material_descriptor_set_pool;

		std::vector<std::unique_ptr<vren::material>> m_materials;
		vren::texture m_white_texture;

		void create_command_pools();

		renderer(renderer_info& info);
		~renderer();

		struct target
		{
			VkFramebuffer m_framebuffer;
			VkRect2D m_render_area;
			VkViewport m_viewport;
			VkRect2D m_scissor;
		};

		// ---------------------------------------------------------------- Texture

		vren::material* create_material();
		vren::material* get_material(size_t idx);

		void render(
			vren::frame& frame,
			vren::renderer::target const& target,
			vren::render_list const& render_list,
			vren::camera const& camera,
			std::vector<VkSemaphore> const& wait_semaphores = {},
			VkFence signal_fence = VK_NULL_HANDLE
		);
	};
}

