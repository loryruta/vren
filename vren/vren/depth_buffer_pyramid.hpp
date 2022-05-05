#pragma once

#include <cstdint>

#include "render_graph.hpp"
#include "vk_helpers/shader.hpp"
#include "vk_helpers/image.hpp"

namespace vren
{
	// Forward decl
	class context;
	class depth_buffer_reductor;

	// ------------------------------------------------------------------------------------------------
	// Depth buffer pyramid
	// ------------------------------------------------------------------------------------------------

	class depth_buffer_pyramid
	{
		friend vren::depth_buffer_reductor;

	public:
		static const uint32_t k_max_depth_buffer_pyramid_level_count = 16;

	private:
		vren::context const* m_context;

		uint32_t m_base_width, m_base_height;
		uint32_t m_level_count;
		vren::vk_utils::image m_image;
		std::vector<vren::vk_image_view> m_image_views;

	public:
		depth_buffer_pyramid(vren::context const& context, uint32_t width, uint32_t height);

	private:
		vren::vk_utils::image create_image();
		std::vector<vren::vk_image_view> create_image_views();

	public:
		inline uint32_t get_image_width(uint32_t level) const
		{
			return m_base_width >> level;
		}

		inline uint32_t get_image_height(uint32_t level) const
		{
			return m_base_height >> level;
		}

		inline uint32_t get_level_count() const
		{
			return m_level_count;
		}

		inline VkImage get_image() const
		{
			return m_image.m_image.m_handle;
		}

		inline VkImageView get_image_view(uint32_t level) const
		{
			return m_image_views.at(level).m_handle;
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Depth buffer reductor
	// ------------------------------------------------------------------------------------------------

	class depth_buffer_reductor
	{
	public:
		static const uint32_t k_workgroup_size_x = 32;
		static const uint32_t k_workgroup_size_y = 32;

	private:
		vren::context const* m_context;

		vren::vk_utils::pipeline m_copy_pipeline;
		vren::vk_utils::pipeline m_reduce_pipeline;

		vren::vk_sampler m_depth_buffer_sampler; // Used to copy the depth buffer in a depth image format (D32 sfloat) to a color image format (R32 sfloat)

	public:
		depth_buffer_reductor(vren::context const& context);

	private:
		vren::vk_utils::pipeline create_copy_pipeline();
		vren::vk_utils::pipeline create_reduce_pipeline();

		vren::vk_sampler create_depth_buffer_sampler();

	private:
		vren::render_graph::node* copy_depth_buffer_to_depth_buffer_pyramid_base(vren::vk_utils::depth_buffer_t const& depth_buffer, vren::depth_buffer_pyramid const& depth_buffer_pyramid) const;
		vren::render_graph::node* reduce_step(vren::depth_buffer_pyramid const& depth_buffer_pyramid, uint32_t current_level) const;
		vren::render_graph::node* reduce(vren::depth_buffer_pyramid const& depth_buffer_pyramid) const;

	public:
		vren::render_graph::node* copy_and_reduce(vren::vk_utils::depth_buffer_t const& depth_buffer, vren::depth_buffer_pyramid const& depth_buffer_pyramid) const;
	};

	// ------------------------------------------------------------------------------------------------

	vren::render_graph::node* blit_depth_buffer_pyramid_level_to_color_buffer( // Debug
		vren::render_graph::allocator& allocator,
		vren::depth_buffer_pyramid const& depth_buffer_pyramid,
		uint32_t level,
		vren::vk_utils::color_buffer_t const& color_buffer,
		uint32_t width,
		uint32_t height
	);
}
