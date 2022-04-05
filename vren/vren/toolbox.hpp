#pragma once

#include <memory>

#include "pools/command_pool.hpp"
#include "pools/descriptor_pool.hpp"
#include "vk_helpers/image.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Toolbox
	// ------------------------------------------------------------------------------------------------

	class toolbox
	{
		friend vren::context;

	private:
		vren::context const* m_context;

		vren::command_pool create_graphics_command_pool(vren::context const& ctx);
		vren::command_pool create_transfer_command_pool(vren::context const& ctx);

		void lazy_initialize();

	public:
		vren::command_pool m_graphics_command_pool;
		vren::command_pool m_transfer_command_pool;
		vren::descriptor_pool m_descriptor_pool;

		std::shared_ptr<vren::vk_utils::texture> m_white_texture;
		std::shared_ptr<vren::vk_utils::texture> m_black_texture;
		std::shared_ptr<vren::vk_utils::texture> m_red_texture;
		std::shared_ptr<vren::vk_utils::texture> m_green_texture;
		std::shared_ptr<vren::vk_utils::texture> m_blue_texture;

		explicit toolbox(vren::context const& ctx);
	};
}
