#pragma once

#include <vector>
#include <queue>
#include <functional>
#include <span>

#include <glm/glm.hpp>

#include "base/base.hpp"
#include "vk_helpers/buffer.hpp"
#include "gpu_repr.hpp"
#include "pool/descriptor_pool.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Light array
	// ------------------------------------------------------------------------------------------------

	class light_array
	{
	public:
		static const size_t k_max_point_light_count = 32768;
		static const size_t k_max_directional_light_count = 32768;
		static const size_t k_max_spot_light_count = 32768;

		static const size_t k_point_light_buffer_size = k_max_point_light_count * sizeof(vren::point_light);
		static const size_t k_directional_light_buffer_size = k_max_directional_light_count * sizeof(vren::directional_light);
		static const size_t k_spot_light_buffer_size = k_max_spot_light_count * sizeof(vren::spot_light);

	private:
		vren::context const* m_context;

	public:
		vren::vk_utils::buffer m_point_light_buffer;
		vren::vk_utils::buffer m_directional_light_buffer;
		vren::vk_utils::buffer m_spot_light_buffer;

		size_t m_point_light_count = 0;
		size_t m_directional_light_count = 0;
		size_t m_spot_light_count = 0;

	public:
		light_array(vren::context const& context);

	private:
		vren::vk_utils::buffer create_buffer(size_t buffer_size, std::string const& buffer_name);

	public:
		inline vren::point_light* get_point_light_buffer_pointer() const
		{
			return (vren::point_light*) m_point_light_buffer.m_allocation_info.pMappedData;
		}

		inline vren::directional_light* get_directional_light_buffer_pointer() const
		{
			return (vren::directional_light*) m_directional_light_buffer.m_allocation_info.pMappedData;
		}

		inline vren::spot_light *get_spot_light_buffer_pointer() const
		{
			return (vren::spot_light*) m_spot_light_buffer.m_allocation_info.pMappedData;
		}

		void write_descriptor_set(VkDescriptorSet descriptor_set) const;
	};
}
