#pragma once

#include <cstdint>

#include "pipeline/render_graph.hpp"
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
		vren::vk_sampler m_sampler;

	public:
		depth_buffer_pyramid(vren::context const& context, uint32_t width, uint32_t height);

	private:
		vren::vk_utils::image create_image();
		std::vector<vren::vk_image_view> create_image_views();
		vren::vk_sampler create_sampler();

	public:
		inline uint32_t get_image_width(uint32_t level) const
		{
			return glm::max(m_base_width >> level, 1u);
		}

		inline uint32_t get_image_height(uint32_t level) const
		{
			return glm::max(m_base_height >> level, 1u);
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

		inline VkSampler get_sampler() const
		{
			return m_sampler.m_handle;
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
		vren::render_graph::graph_t copy_depth_buffer_to_depth_buffer_pyramid_base(
			vren::render_graph::allocator& allocator,
			vren::vk_utils::depth_buffer_t const& depth_buffer,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid
		) const;

		vren::render_graph::graph_t reduce_step(
			vren::render_graph::allocator& allocator,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid,
			uint32_t current_level
		) const;

		vren::render_graph::graph_t reduce(
			vren::render_graph::allocator& allocator,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid
		) const;

	public:
		vren::render_graph::graph_t copy_and_reduce(
			vren::render_graph::allocator& allocator,
			vren::vk_utils::depth_buffer_t const& depth_buffer,
			vren::depth_buffer_pyramid const& depth_buffer_pyramid
		) const;
	};

	// ------------------------------------------------------------------------------------------------

	vren::render_graph::graph_t blit_depth_buffer_pyramid_level_to_color_buffer( // Debug
		vren::render_graph::allocator& allocator,
		vren::depth_buffer_pyramid const& depth_buffer_pyramid,
		uint32_t level,
		vren::vk_utils::color_buffer_t const& color_buffer,
		uint32_t width,
		uint32_t height
	);
}
