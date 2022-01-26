#pragma once

#include <memory>
#include <array>
#include <filesystem>

#include "config.hpp"
#include "simple_draw.hpp"
#include "render_list.hpp"
#include "gpu_allocator.hpp"
#include "frame.hpp"
#include "material.hpp"
#include "descriptor_set_pool.hpp"
#include "debug_gui.hpp"

#include <glm/glm.hpp>

#ifdef NSIGHT_AFTERMATH
	#include "NsightAftermath/NsightAftermathGpuCrashTracker.h"
#endif

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
		glm::vec3 m_position;
		float _pad;
		glm::mat4 m_view;
		glm::mat4 m_projection;
	};

	class renderer : public std::enable_shared_from_this<renderer>
	{
	public:
		static VkFormat const k_color_output_format = VK_FORMAT_B8G8R8A8_UNORM;

		struct queue_families
		{
			uint32_t m_graphics_idx = -1;
			uint32_t m_compute_idx  = -1;
			uint32_t m_transfer_idx = -1;

			inline bool is_valid() const
			{
				return m_graphics_idx >= 0 && m_compute_idx >= 0 && m_transfer_idx >= 0;
			}
		};

		struct target
		{
			VkFramebuffer m_framebuffer;
			VkRect2D m_render_area;
			VkViewport m_viewport;
			VkRect2D m_scissor;
		};

	private:
		explicit renderer(vren::renderer_info& info);

		void _initialize();

		VkInstance create_instance();
		VkDebugUtilsMessengerEXT setup_debug_messenger();
		vren::renderer::queue_families get_queue_families(VkPhysicalDevice physical_device);
		VkPhysicalDevice find_physical_device();
		VkDevice create_logical_device();
		std::vector<VkQueue> get_queues();
		void create_vma_allocator();
		VkRenderPass create_render_pass();
		void create_command_pools();


	public:
		vren::renderer_info m_info;

		VkInstance m_instance;

		// Debug messenger
		VkDebugUtilsMessengerEXT m_debug_messenger;

		// Nvidia Nsight Aftermath
#ifdef NSIGHT_AFTERMATH
		GpuCrashTracker m_gpu_crash_tracker;
#endif

		VkPhysicalDevice m_physical_device;
		VkPhysicalDeviceProperties m_physical_device_properties;

		vren::renderer::queue_families m_queue_families;
		VkDevice m_device;

		std::vector<VkQueue> m_queues;
		VkQueue m_transfer_queue;
		VkQueue m_graphics_queue;
		VkQueue m_compute_queue;

		VmaAllocator m_vma_allocator;

		VkRenderPass m_render_pass;
		VkClearColorValue m_clear_color = {1.0f, 0.0f, 0.0f, 1.0f};

		// Command pools
		VkCommandPool m_graphics_command_pool;
		VkCommandPool m_transfer_command_pool;

		// Subpasses
		std::unique_ptr<vren::simple_draw_pass> m_simple_draw_pass;

		std::unique_ptr<vren::descriptor_set_pool> m_descriptor_set_pool;

		// Textures
		std::shared_ptr<vren::texture> m_white_texture;
		std::shared_ptr<vren::texture> m_black_texture;
		std::shared_ptr<vren::texture> m_red_texture;
		std::shared_ptr<vren::texture> m_green_texture;
		std::shared_ptr<vren::texture> m_blue_texture;

		std::unique_ptr<vren::debug_gui> m_debug_gui;

		~renderer();

		void render(
			vren::frame& frame,
			vren::renderer::target const& target,
			vren::render_list const& render_list,
			vren::lights_array const& lights_array,
			vren::camera const& camera,
			std::vector<VkSemaphore> const& wait_semaphores = {},
			VkFence signal_fence = VK_NULL_HANDLE
		);

		static std::shared_ptr<vren::renderer> create(vren::renderer_info& info); // renderer's initialization must be achieved in two steps because of shared_from_this()
	};
}

