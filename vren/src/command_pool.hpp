#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "context.hpp"
#include "utils/vk_raii.hpp"

namespace vren
{
	// Forward decl
	class vk_command_buffer;

	// --------------------------------------------------------------------------------------------------------------------------------
	// Command pool
	// --------------------------------------------------------------------------------------------------------------------------------

	class command_pool : public std::enable_shared_from_this<command_pool>
	{
		friend vren::vk_command_buffer;

	private:
		std::shared_ptr<vren::context> m_context;
		VkCommandPool m_handle;

		std::vector<VkCommandBuffer> m_initial_state_cmd_buffers;

		void _release_command_buffer(vren::vk_command_buffer const& cmd_buf);

	public:
		command_pool(std::shared_ptr<vren::context> const& ctx, VkCommandPool handle);
		command_pool(command_pool const& other) = delete;
		~command_pool();

		vren::vk_command_buffer acquire_command_buffer();
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// Command buffer
	// --------------------------------------------------------------------------------------------------------------------------------

	class vk_command_buffer
	{
		friend vren::command_pool;

	public:
		VkCommandBuffer m_handle = VK_NULL_HANDLE;

	private:
		std::shared_ptr<command_pool> m_command_pool;

		vk_command_buffer(std::shared_ptr<command_pool> const& cmd_pool, VkCommandBuffer handle);
		vk_command_buffer(vk_command_buffer const& other) = delete;

	public:
		vk_command_buffer(vk_command_buffer&& other);
		~vk_command_buffer();

		vk_command_buffer& operator=(vk_command_buffer const& other) = delete;
		vk_command_buffer& operator=(vk_command_buffer&& other);
	};
}
