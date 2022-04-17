#pragma once

#include <vector>

#include "vk_helpers/image.hpp"
#include "pools/descriptor_pool.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Texture manager descriptor pool
	// ------------------------------------------------------------------------------------------------

	class texture_manager_descriptor_pool : public vren::descriptor_pool
	{
	protected:
		VkDescriptorSet allocate_descriptor_set(VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout) override;

	public:
		explicit texture_manager_descriptor_pool(vren::context const& context, uint32_t max_sets, std::span<VkDescriptorPoolSize> const& pool_sizes);
	};

	// ------------------------------------------------------------------------------------------------
	// Texture manager
	// ------------------------------------------------------------------------------------------------

	class texture_manager
	{
	public:
		static constexpr uint32_t k_max_texture_count = 65536;

	private:
		vren::context const* m_context;
		vren::vk_descriptor_set_layout m_descriptor_set_layout;
		vren::texture_manager_descriptor_pool m_descriptor_pool;

		vren::vk_descriptor_set_layout create_descriptor_set_layout();
		vren::texture_manager_descriptor_pool create_descriptor_pool();
	public:
		std::vector<vren::vk_utils::texture> m_textures;
		std::shared_ptr<vren::pooled_vk_descriptor_set> m_descriptor_set;

		explicit texture_manager(vren::context const& context);

		void rewrite_descriptor_set();
	};
}
