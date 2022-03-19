#pragma once

#include <memory>
#include <array>

#include <imgui.h>

#include "context.hpp"
#include "renderer.hpp"

namespace vren_demo::ui
{
	// ------------------------------------------------------------------------------------------------
	// vk_pool_info_ui
	// ------------------------------------------------------------------------------------------------

	class vk_pool_info_ui
	{
	private:
	public:
		void show();
	};

	// ------------------------------------------------------------------------------------------------
	// main_ui
	// ------------------------------------------------------------------------------------------------

	class main_ui
	{
	private:
		std::shared_ptr<vren::context> m_context;
		std::shared_ptr<vren::renderer> m_renderer;

	public:
		main_ui(
			std::shared_ptr<vren::context> const& ctx,
			std::shared_ptr<vren::renderer> const& renderer
		);

		void show_vk_pool_info_ui();
		void show();
	};


}
