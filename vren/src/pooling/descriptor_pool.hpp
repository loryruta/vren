#pragma once

#include <memory>
#include <vector>
#include <queue>

#include <vulkan/vulkan.h>

#include "context.hpp"

namespace vren // TODO convert to object_pool
{
	// Forward decl
	class vk_descriptor_set;

	// --------------------------------------------------------------------------------------------------------------------------------
	// descriptor_pool
	// --------------------------------------------------------------------------------------------------------------------------------

	class descriptor_pool : public std::enable_shared_from_this<descriptor_pool>
	{
	private:
		void _release_descriptor_set(vren::vk_descriptor_set const& desc_set);
		void _alloc_descriptor_pool();

		friend vren::vk_descriptor_set;

	protected:
		virtual VkDescriptorPool create_descriptor_pool(int max_sets) = 0;

	public:
		std::shared_ptr<vren::context> m_context;

		std::vector<VkDescriptorPool> m_descriptor_pools;
		size_t m_last_pool_allocated_count = 0;

		std::vector<VkDescriptorSet> m_pooled_descriptor_sets;

		explicit descriptor_pool(std::shared_ptr<vren::context> const& ctx);
		~descriptor_pool();

		vren::vk_descriptor_set acquire_descriptor_set(VkDescriptorSetLayout desc_set_layout);
	};

	// --------------------------------------------------------------------------------------------------------------------------------
	// vk_descriptor_set
	// --------------------------------------------------------------------------------------------------------------------------------

	class vk_descriptor_set
	{
	private:
		vk_descriptor_set(
			std::shared_ptr<descriptor_pool> const& descriptor_pool,
			VkDescriptorSet handle
		);

		friend vren::descriptor_pool;

	public:
		std::shared_ptr<descriptor_pool> m_descriptor_pool;
		VkDescriptorSet m_handle;

		vk_descriptor_set(vk_descriptor_set const& other) = delete;
		vk_descriptor_set(vk_descriptor_set&& other);
		~vk_descriptor_set();

		vk_descriptor_set& operator=(vk_descriptor_set const& other) = delete;
		vk_descriptor_set& operator=(vk_descriptor_set&& other);
	};
}
