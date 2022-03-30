#pragma once

#include <memory>

#include "pooling/command_pool.hpp"
#include "pooling/fence_pool.hpp"
#include "utils/image.hpp"
#include "light_array.hpp"

namespace vren::vk_utils
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// toolbox
	// ------------------------------------------------------------------------------------------------

	class toolbox
	{
	private:
		explicit toolbox(std::shared_ptr<vren::context> const& ctx);

		std::shared_ptr<vren::command_pool> _create_graphics_command_pool(std::shared_ptr<vren::context> const& ctx);
		std::shared_ptr<vren::command_pool> _create_transfer_command_pool(std::shared_ptr<vren::context> const& ctx);

	public:
		std::shared_ptr<vren::context> m_context;

		/* Light array */
		std::shared_ptr<vren::vk_descriptor_set_layout>    m_light_array_descriptor_set_layout;
		std::shared_ptr<vren::light_array_descriptor_pool> m_light_array_descriptor_pool;

		/* Pools */
		std::shared_ptr<vren::command_pool> m_graphics_command_pool;
		std::shared_ptr<vren::command_pool> m_transfer_command_pool;
		std::shared_ptr<vren::fence_pool>   m_fence_pool;

		/* Default textures */
		std::shared_ptr<vren::vk_utils::texture> m_white_texture;
		std::shared_ptr<vren::vk_utils::texture> m_black_texture;
		std::shared_ptr<vren::vk_utils::texture> m_red_texture;
		std::shared_ptr<vren::vk_utils::texture> m_green_texture;
		std::shared_ptr<vren::vk_utils::texture> m_blue_texture;

		static std::shared_ptr<toolbox> create(std::shared_ptr<vren::context> const& ctx);
	};
}
