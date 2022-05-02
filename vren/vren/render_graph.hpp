#pragma once

#include <cassert>
#include <vector>
#include <functional>
#include <optional>

#include <volk.h>

#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class render_graph_executor;
	class render_graph_handler;

	// ------------------------------------------------------------------------------------------------
	// Image resource
	// ------------------------------------------------------------------------------------------------

	struct render_graph_node_image_resource
	{
		std::string m_name = "unnamed";

		VkImage m_image;
		VkImageAspectFlags m_image_aspect;
		uint32_t m_mip_level = 0;
		uint32_t m_layer = 0;

		inline bool operator==(render_graph_node_image_resource const& other) const
		{
			return m_image == other.m_image && (m_image_aspect & other.m_image_aspect) && m_mip_level == other.m_mip_level && m_layer == other.m_layer;
		}
	};

	struct render_graph_node_image_resource_hasher
	{
		uint64_t operator()(render_graph_node_image_resource const& value) const
		{
			return uint64_t(value.m_image) ^ value.m_image_aspect ^ (uint64_t(value.m_mip_level) << 32) ^ (value.m_layer);
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Buffer resource
	// ------------------------------------------------------------------------------------------------

	struct render_graph_node_buffer_resource
	{
		std::string m_name = "unnamed";

		VkBuffer m_buffer;
		VkDeviceSize m_size;
		VkDeviceSize m_offset;

		bool operator==(render_graph_node_buffer_resource const& other) const;
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph node
	// ------------------------------------------------------------------------------------------------

	class render_graph_node
	{
		friend vren::render_graph_executor;
		friend vren::render_graph_handler;

	public:
		using callback_t = std::function<void(uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)>;

	private:
		std::string m_name = "unnamed";

		VkPipelineStageFlags m_in_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags m_out_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		std::vector<std::tuple<render_graph_node_image_resource, VkImageLayout, VkAccessFlags>> m_image_resources;
		std::vector<std::pair<render_graph_node_buffer_resource, VkAccessFlags>> m_buffer_resources;

		callback_t m_callback;

		std::vector<render_graph_node*> m_previous_nodes;
		std::vector<render_graph_node*> m_following_nodes;

	public:
		explicit render_graph_node() = default;

		inline auto set_name(std::string const& name)
		{
			m_name = name;
			return this;
		}

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

		inline auto add_image(render_graph_node_image_resource&& image_resource, VkImageLayout image_layout, VkAccessFlags access_flags)
		{
			m_image_resources.emplace_back(image_resource, image_layout, access_flags);
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

		inline auto chain(render_graph_node* following)
		{
			add_following(following);

			return following;
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
		void initialize_image_layout(vren::render_graph_node const& node, std::tuple<vren::render_graph_node_image_resource, VkImageLayout, VkAccessFlags> const& image_resource, VkCommandBuffer command_buffer);
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
		render_graph_t m_handle;

	public:
		explicit render_graph_handler() = default;
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

	render_graph_node* clear_color_buffer(VkImage color_buffer, VkClearColorValue clear_color_value);
	render_graph_node* clear_depth_stencil_buffer(VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value);
}

