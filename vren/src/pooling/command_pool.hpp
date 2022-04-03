#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "object_pool.hpp"
#include "utils/vk_raii.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Command pool
	// ------------------------------------------------------------------------------------------------

	using pooled_vk_command_buffer = pooled_object<VkCommandBuffer>;

    class command_pool : public vren::object_pool<VkCommandBuffer>
	{
	private:
		vren::context const* m_context;
		vren::vk_command_pool m_command_pool;

	public:
		explicit command_pool(vren::context const& ctx, vren::vk_command_pool&& cmd_pool);

		pooled_vk_command_buffer acquire();
	};
}
