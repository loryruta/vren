#pragma once

#include <memory>
#include <array>
#include <vector>
#include <filesystem>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

#include "config.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Forward decl
// --------------------------------------------------------------------------------------------------------------------------------

namespace vren
{
	class command_pool;
    class fence_pool;
}

namespace vren::vk_utils
{
	struct texture;
}

// --------------------------------------------------------------------------------------------------------------------------------

namespace vren
{
	void get_supported_layers(std::vector<VkLayerProperties>& layers); // todo in utils/misc ?
	bool does_support_layers(std::vector<char const*> const& layers);

	struct context_info
	{
		char const* m_app_name;
		uint32_t m_app_version;
		std::vector<char const*> m_layers;
		std::vector<char const*> m_extensions;
		std::vector<char const*> m_device_extensions;
	};

	class context : public std::enable_shared_from_this<context>
	{
	public:
		struct queue_families
		{
			uint32_t m_graphics_idx = -1;
			uint32_t m_compute_idx  = -1;
			uint32_t m_transfer_idx = -1;
			uint32_t m_present_idx  = -1;

			inline bool is_valid() const
			{
				return m_graphics_idx >= 0 && m_compute_idx >= 0 && m_transfer_idx >= 0;
			}
		};

	private:
		explicit context(vren::context_info& info);

		void _init();

		VkInstance create_instance();
		VkDebugUtilsMessengerEXT setup_debug_messenger();
		vren::context::queue_families get_queue_families(VkPhysicalDevice physical_device);
		VkPhysicalDevice find_physical_device();
		VkDevice create_logical_device();
		std::vector<VkQueue> get_queues();
		void create_vma_allocator();
		void _init_graphics_command_pool();
		void _init_transfer_command_pool();

	public:
		vren::context_info m_info;

		VkInstance m_instance;

		// Debug messenger
		VkDebugUtilsMessengerEXT m_debug_messenger;

		VkPhysicalDevice m_physical_device;
		VkPhysicalDeviceProperties m_physical_device_properties;

		vren::context::queue_families m_queue_families;
		VkDevice m_device;

		std::vector<VkQueue> m_queues;
		VkQueue m_transfer_queue;
		VkQueue m_graphics_queue;
		VkQueue m_compute_queue;

		VmaAllocator m_vma_allocator;

		// Command pools
		std::shared_ptr<vren::command_pool> m_graphics_command_pool;
		std::shared_ptr<vren::command_pool> m_transfer_command_pool;

        std::shared_ptr<vren::fence_pool> m_fence_pool;

		// Textures
		std::shared_ptr<vren::vk_utils::texture> m_white_texture;
		std::shared_ptr<vren::vk_utils::texture> m_black_texture;
		std::shared_ptr<vren::vk_utils::texture> m_red_texture;
		std::shared_ptr<vren::vk_utils::texture> m_green_texture;
		std::shared_ptr<vren::vk_utils::texture> m_blue_texture;

		~context();

		static std::shared_ptr<vren::context> create(vren::context_info& info); // renderer's initialization must be achieved in two steps because of shared_from_this()
	};
}

