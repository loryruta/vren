#pragma once

#include <memory>

#include "context.hpp"
#include "vk_raii.hpp"
#include "command_pool.hpp"

namespace vren::vk_utils
{
	void check(VkResult result);

	vren::vk_semaphore create_semaphore(
		std::shared_ptr<vren::context> const& ctx
	);

	vren::vk_fence create_fence(
		std::shared_ptr<vren::context> const& ctx
	);

	void submit_command_buffer(
		VkQueue queue,
		std::vector<VkSemaphore> const& wait_semaphores,
		std::vector<VkPipelineStageFlags> const& wait_stages,
		vren::vk_command_buffer const& cmd_buf,
		std::vector<VkSemaphore> const& signal_semaphores,
		VkFence signal_fence
	);


}
