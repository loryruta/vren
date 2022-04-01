#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "object_pool.hpp"
#include "utils/vk_raii.hpp"

namespace vren
{
	// ------------------------------------------------------------------------------------------------
	// command_pool
	// ------------------------------------------------------------------------------------------------

	using pooled_vk_command_buffer = pooled_object<VkCommandBuffer>;

    class command_pool : public vren::object_pool<VkCommandBuffer>
	{
	private:
		std::shared_ptr<vren::context> m_context;
		vren::vk_command_pool m_handle;

		explicit command_pool(
			std::shared_ptr<vren::context> const& ctx,
			vren::vk_command_pool&& handle
		);

	public:
		pooled_vk_command_buffer acquire();

		static std::shared_ptr<vren::command_pool> create(std::shared_ptr<vren::context> const& ctx, vren::vk_command_pool&& handle)
		{
			return std::shared_ptr<vren::command_pool>(new vren::command_pool(ctx, std::move(handle)));
		}
	};
}
