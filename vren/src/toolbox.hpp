#pragma once

#include <memory>

#include "pooling/command_pool.hpp"
#include "pooling/descriptor_pool.hpp"
#include "pooling/fence_pool.hpp"
#include "utils/image.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Toolbox
	// ------------------------------------------------------------------------------------------------

	class toolbox
	{
	private:
		vren::command_pool create_graphics_command_pool(vren::context const& ctx);
		vren::command_pool create_transfer_command_pool(vren::context const& ctx);

		vren::context const* m_context;

	public:
		vren::command_pool m_graphics_command_pool;
		vren::command_pool m_transfer_command_pool;
		vren::fence_pool m_fence_pool;
		vren::descriptor_pool m_descriptor_pool;

		std::shared_ptr<vren::vk_utils::texture> m_white_texture;
		std::shared_ptr<vren::vk_utils::texture> m_black_texture;
		std::shared_ptr<vren::vk_utils::texture> m_red_texture;
		std::shared_ptr<vren::vk_utils::texture> m_green_texture;
		std::shared_ptr<vren::vk_utils::texture> m_blue_texture;

		explicit toolbox(vren::context const& ctx);
	};
}
