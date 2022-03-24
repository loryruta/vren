#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "object_pool.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// command_pool
	// ------------------------------------------------------------------------------------------------

    class command_pool : public vren::object_pool<VkCommandBuffer>
	{
	private:
		std::shared_ptr<vren::context> m_context;
		VkCommandPool m_handle;

		explicit command_pool(std::shared_ptr<vren::context> const& ctx, VkCommandPool handle);

    protected:
        VkCommandBuffer create_object() override;
        void release(VkCommandBuffer&& cmd_buf) override;

	public:
		~command_pool();

		static std::shared_ptr<vren::command_pool> create(std::shared_ptr<vren::context> const& ctx, VkCommandPool handle);
	};

	using pooled_vk_command_buffer = pooled_object<VkCommandBuffer>;
}
