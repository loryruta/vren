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
#include "primitives/bucket_sort.hpp"
#include "primitives/build_bvh.hpp"

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

		// Reduce
		vren::reduce m_reduce_uint_add;
		vren::reduce m_reduce_uint_min;
		vren::reduce m_reduce_uint_max;
		vren::reduce m_reduce_vec3_add;
		vren::reduce m_reduce_vec3_min;
		vren::reduce m_reduce_vec3_max;

		vren::blelloch_scan m_blelloch_scan;
		vren::radix_sort m_radix_sort;
		vren::bucket_sort m_bucket_sort;

		vren::build_bvh m_build_bvh;

		explicit toolbox(vren::context const& ctx);
	};
}
