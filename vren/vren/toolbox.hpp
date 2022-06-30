#pragma once

#include <memory>

#include "pool/command_pool.hpp"
#include "pool/descriptor_pool.hpp"
#include "vk_helpers/image.hpp"
#include "texture_manager.hpp"
#include "material.hpp"
#include "primitives/reduce.hpp"
#include "primitives/blelloch_scan.hpp"
#include "primitives/radix_sort.hpp"

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

		vren::reduce m_reduce;
		vren::blelloch_scan m_blelloch_scan;
		vren::radix_sort m_radix_sort;

		explicit toolbox(vren::context const& ctx);
	};
}
