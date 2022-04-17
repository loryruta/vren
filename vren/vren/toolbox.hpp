#pragma once

#include <memory>

#include "pools/command_pool.hpp"
#include "pools/descriptor_pool.hpp"
#include "vk_helpers/image.hpp"
#include "texture_manager.hpp"
#include "material.hpp"

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

		vren::command_pool create_graphics_command_pool();
		vren::command_pool create_transfer_command_pool();
		vren::descriptor_pool create_descriptor_pool();

		void lazy_initialize();

	public:
		vren::command_pool m_graphics_command_pool;
		vren::command_pool m_transfer_command_pool;
		vren::descriptor_pool m_descriptor_pool; // General purpose descriptor pool

		vren::texture_manager m_texture_manager;
		vren::material_manager m_material_manager;

		std::shared_ptr<vren::vk_utils::texture> m_white_texture;
		std::shared_ptr<vren::vk_utils::texture> m_black_texture;
		std::shared_ptr<vren::vk_utils::texture> m_red_texture;
		std::shared_ptr<vren::vk_utils::texture> m_green_texture;
		std::shared_ptr<vren::vk_utils::texture> m_blue_texture;

		explicit toolbox(vren::context const& ctx);
	};
}
