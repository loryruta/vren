#pragma once

#include <vector>
#include <functional>
#include <optional>

#include <volk.h>

#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class render_graph_executor;

	// ------------------------------------------------------------------------------------------------
	// Image resource
	// ------------------------------------------------------------------------------------------------

	struct render_graph_node_image_resource
	{
		VkImage m_image;
		VkImageLayout m_image_layout;
		VkImageSubresourceRange m_image_subresource_range;

		bool operator==(render_graph_node_image_resource const& other) const;
	};

	inline auto describe_image(VkImage image, VkImageLayout image_layout, VkImageAspectFlags image_aspect, uint32_t mip_level = 0, uint32_t layer = 0)
	{
		VkImageSubresourceRange subresource_range = { .aspectMask = image_aspect, .baseMipLevel = mip_level, .levelCount = 1, .baseArrayLayer = layer, .layerCount = 1 };
		return render_graph_node_image_resource{ .m_image = image, .m_image_layout = image_layout, .m_image_subresource_range = subresource_range };
	}

	// ------------------------------------------------------------------------------------------------
	// Buffer resource
	// ------------------------------------------------------------------------------------------------

	struct render_graph_node_buffer_resource
	{
		VkBuffer m_buffer;
		VkDeviceSize m_size;
		VkDeviceSize m_offset;

		bool operator==(render_graph_node_buffer_resource const& other) const;
	};

	inline auto describe_buffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0)
	{
		return render_graph_node_buffer_resource{ .m_buffer = buffer, .m_size = size, .m_offset = offset };
	}

	// ------------------------------------------------------------------------------------------------
	// Render-graph node
	// ------------------------------------------------------------------------------------------------

	class render_graph_node
	{
		friend vren::render_graph_executor;

	public:
		using callback_t = std::function<void(uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)>;

	private:
		VkPipelineStageFlags m_in_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags m_out_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		std::vector<std::pair<render_graph_node_image_resource, VkAccessFlags>> m_image_resources;
		std::vector<std::pair<render_graph_node_buffer_resource, VkAccessFlags>> m_buffer_resources;

		callback_t m_callback;

		std::vector<render_graph_node*> m_previous_nodes;
		std::vector<render_graph_node*> m_following_nodes;

	public:
		explicit render_graph_node() = default;

		inline auto set_in_stage(VkPipelineStageFlags stage)
		{
			m_in_stage = stage;
			return this;
		}

		inline auto set_out_stage(VkPipelineStageFlags stage)
		{
			m_out_stage = stage;
			return this;
		}

		inline auto add_image(render_graph_node_image_resource&& image_resource, VkAccessFlags access_flags)
		{
			m_image_resources.emplace_back(image_resource, access_flags);
			return this;
		}

		inline auto add_buffer(render_graph_node_buffer_resource&& buffer_resource, VkAccessFlags access_flags)
		{
			m_buffer_resources.emplace_back(buffer_resource, access_flags);
			return this;
		}

		inline auto set_callback(callback_t const& callback)
		{
			m_callback = callback;
			return this;
		}

		inline std::vector<render_graph_node*> const& get_previous_nodes() const
		{
			return m_previous_nodes;
		}

		inline std::vector<render_graph_node*> const& get_following_nodes() const
		{
			return m_following_nodes;
		}

	public:
		inline auto add_following(render_graph_node* following)
		{
			assert(following != nullptr);
			assert(this != following);

			m_following_nodes.push_back(following);
			following->m_previous_nodes.push_back(this);

			return this;
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph
	// ------------------------------------------------------------------------------------------------

	using render_graph_t = std::vector<render_graph_node*>;

	// ------------------------------------------------------------------------------------------------
	// Render-graph executor
	// ------------------------------------------------------------------------------------------------

	class render_graph_executor // This class exists only to have access to render_graph_node private members
	{
	public:
		explicit render_graph_executor() = default;

	private:
		void place_barriers(vren::render_graph_node const& node, VkCommandBuffer command_buffer);
		void execute_node(vren::render_graph_node const& node, uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container);

	public:
		void execute(vren::render_graph_t const& render_graph, uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container);
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph handler
	// ------------------------------------------------------------------------------------------------

	class render_graph_handler
	{
	private:
		render_graph_t const& m_handle;

	public:
		explicit render_graph_handler(render_graph_t const& handle) :
			m_handle(std::move(handle))
		{}
		~render_graph_handler();

		inline render_graph_t const& get_handle() const
		{
			return m_handle;
		}
	};

	// ------------------------------------------------------------------------------------------------

	render_graph_node* clear_color_buffer(VkImage color_buffer, VkImageSubresourceRange subresource_range, VkClearColorValue clear_color_value);
	render_graph_node* clear_depth_stencil_buffer(VkImage depth_buffer, VkImageSubresourceRange subresource_range, VkClearDepthStencilValue clear_depth_stencil_value);
}

