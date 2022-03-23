#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "context.hpp"
#include "object_pool.hpp"

namespace vren
{
    using pooled_vk_command_buffer = pooled_object<VkCommandBuffer>;

    class command_pool : public vren::object_pool<VkCommandBuffer>
	{
	private:
		std::shared_ptr<vren::context> m_context;
		VkCommandPool m_handle;

    protected:
        VkCommandBuffer create_object() override;
        void release(VkCommandBuffer&& cmd_buf) override;

	public:
		command_pool(std::shared_ptr<vren::context> ctx, VkCommandPool handle);
		command_pool(command_pool const& other) = delete;
		~command_pool();
	};
}
