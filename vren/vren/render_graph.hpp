#pragma once

#include <cassert>
#include <cstring>
#include <vector>
#include <array>
#include <functional>
#include <unordered_set>
#include <span>

#include <volk.h>

#include "base/resource_container.hpp"
#include "utils/log.hpp"

namespace vren::render_graph
{
	// Forward decl
	class allocator;

	// ------------------------------------------------------------------------------------------------

	inline constexpr size_t k_max_name_length = 64;

	// ------------------------------------------------------------------------------------------------
	// Image access
	// ------------------------------------------------------------------------------------------------

	struct image_access
	{
		static constexpr size_t k_max_name_length = vren::render_graph::k_max_name_length;

		char m_name[k_max_name_length] = "unnamed"; // TODO char const*

		VkImage m_image = VK_NULL_HANDLE;
		VkImageAspectFlags m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		uint32_t m_mip_level = 0;
		uint32_t m_layer = 0;

		VkImageLayout m_image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkAccessFlags m_access_flags = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

		inline bool operator==(vren::render_graph::image_access const& other) const
		{
			return std::strcmp(m_name, other.m_name) && m_image == other.m_image && m_image_aspect == other.m_image_aspect && m_mip_level == other.m_mip_level && m_layer == other.m_layer;
		}

		inline bool does_conflict(vren::render_graph::image_access const& other) const
		{
			return m_image == other.m_image && (m_image_aspect & other.m_image_aspect) && m_mip_level == other.m_mip_level && m_layer == other.m_layer;
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Buffer access
	// ------------------------------------------------------------------------------------------------

	struct buffer_access
	{
		static constexpr size_t k_max_name_length = vren::render_graph::k_max_name_length;

		char m_name[k_max_name_length] = "unnamed"; // TODO char const*

		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceSize m_size = VK_WHOLE_SIZE;
		VkDeviceSize m_offset = 0;

		VkAccessFlags m_access_flags = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

		inline bool operator==(vren::render_graph::buffer_access const& other) const
		{
			return std::strcmp(m_name, other.m_name) && m_buffer == other.m_buffer && m_size == other.m_size && m_offset == other.m_offset;
		}

		inline bool does_conflict(vren::render_graph::buffer_access const& other) const
		{
			return m_buffer == other.m_buffer; // IMPROVEMENT check for offset and size
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph node
	// ------------------------------------------------------------------------------------------------

	using node_index_t = uint8_t;

	class node
	{
		friend vren::render_graph::allocator;

	public:
		static constexpr size_t k_max_name_length = vren::render_graph::k_max_name_length;
		static constexpr size_t k_max_per_resource_accesses = 32;
		static constexpr size_t k_max_image_accesses = k_max_per_resource_accesses;
		static constexpr size_t k_max_buffer_accesses = k_max_per_resource_accesses;
		static constexpr size_t k_max_previous_nodes = 4;
		static constexpr size_t k_max_next_nodes = 4;

		using access_index_t = uint8_t;
		using image_access_index_t = access_index_t;
		using buffer_access_index_t = access_index_t;

		using callback_t = std::function<void(uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)>;

		static_assert(std::numeric_limits<image_access_index_t>::max() >= k_max_image_accesses - 1);
		static_assert(std::numeric_limits<buffer_access_index_t>::max() >= k_max_buffer_accesses - 1);
		static_assert(std::numeric_limits<node_index_t>::max() >= k_max_previous_nodes - 1);
		static_assert(std::numeric_limits<node_index_t>::max() >= k_max_next_nodes - 1);

	private:
		vren::render_graph::allocator* m_allocator;
		node_index_t m_idx = -1;

		char m_name[k_max_name_length] = "unnamed"; // TODO char const*

		VkPipelineStageFlags m_src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags m_dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		std::array<vren::render_graph::image_access, k_max_image_accesses> m_image_accesses;
		access_index_t m_next_image_access_idx = 0;

		std::array<vren::render_graph::buffer_access, k_max_buffer_accesses> m_buffer_accesses;
		access_index_t m_next_buffer_access_idx = 0;

		callback_t m_callback;

		std::array<node_index_t, k_max_previous_nodes> m_previous_nodes;
		node_index_t m_next_previous_node_idx = 0;

		std::array<node_index_t, k_max_next_nodes> m_next_nodes;
		node_index_t m_next_next_node_idx = 0;

	public:
		explicit node() = default;

		inline uint32_t get_idx() const
		{
			return m_idx;
		}

		inline auto get_name() const { return m_name; }
		inline void set_name(char const* name) { strcpy_s(m_name, name); }

		inline auto get_src_stage() const { return m_src_stage; }
		inline void set_src_stage(VkPipelineStageFlags stage) { m_src_stage = stage; }

		inline auto get_dst_stage() const { return m_dst_stage; }
		inline void set_dst_stage(VkPipelineStageFlags stage) { m_dst_stage = stage; }

		inline void add_image(vren::render_graph::image_access&& image_access)
		{
			assert(m_next_image_access_idx < k_max_image_accesses);

			m_image_accesses[m_next_image_access_idx++] = std::move(image_access);
		}

		inline auto get_image_accesses() const -> std::span<vren::render_graph::image_access const>
		{
			return std::span(m_image_accesses.data(), m_next_image_access_idx);
		}

		inline void add_buffer_access(vren::render_graph::buffer_access&& buffer_access)
		{
			assert(m_next_buffer_access_idx < k_max_buffer_accesses);

			m_buffer_accesses[m_next_buffer_access_idx++] = std::move(buffer_access);
		}

		inline auto get_buffer_accesses() const -> std::span<vren::render_graph::buffer_access const>
		{
			return std::span(m_buffer_accesses.data(), m_next_buffer_access_idx);
		}

		inline void operator()(uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container) const
		{
			if (m_callback) {
				m_callback(frame_idx, command_buffer, resource_container);
			}
		}

		inline void set_callback(callback_t const& callback) { m_callback = callback; }

		inline auto get_previous_nodes() const -> std::span<vren::render_graph::node_index_t const>
		{
			return std::span(m_previous_nodes.data(), m_next_previous_node_idx);
		}

		inline auto get_next_nodes() const -> std::span<vren::render_graph::node_index_t const>
		{
			return std::span(m_next_nodes.data(), m_next_next_node_idx);
		}

	public:
		inline auto add_next(vren::render_graph::node* next_node)
		{
			assert(next_node != nullptr);
			assert(this != next_node);
			assert(next_node->m_next_previous_node_idx < k_max_previous_nodes);
			assert(m_next_next_node_idx < k_max_next_nodes);

			next_node->m_previous_nodes[next_node->m_next_previous_node_idx++] = m_idx;
			m_next_nodes[m_next_next_node_idx++] = next_node->m_idx;

			return this;
		}

		vren::render_graph::node* chain(vren::render_graph::node* chain);
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph
	// ------------------------------------------------------------------------------------------------

	using graph_t = std::span<vren::render_graph::node*>;

	void traverse(
		vren::render_graph::allocator& allocator,
		vren::render_graph::graph_t const& graph,
		bool callback_starting_nodes,
		std::function<bool(vren::render_graph::node* node)> const& callback
	);

	void iterate_starting_nodes(
		vren::render_graph::allocator& allocator,
		vren::render_graph::graph_t const& graph,
		std::function<void(vren::render_graph::node*)> const& callback
	);

	void iterate_ending_nodes(
		vren::render_graph::allocator& allocator,
		vren::render_graph::graph_t const& graph,
		std::function<void(vren::render_graph::node*)> const& callback
	);

	// ------------------------------------------------------------------------------------------------
	// Render-graph execution
	// ------------------------------------------------------------------------------------------------

	void execute(vren::render_graph::allocator& allocator, vren::render_graph::graph_t const& graph, uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container);

	// ------------------------------------------------------------------------------------------------
	// Render-graph allocator
	// ------------------------------------------------------------------------------------------------

	class allocator
	{
	public:
		static constexpr size_t k_max_allocable_nodes = 256;

		static_assert(std::numeric_limits<vren::render_graph::node_index_t>::max() >= k_max_allocable_nodes - 1);

	private:
		std::vector<vren::render_graph::node> m_nodes;
		uint32_t m_next_node_idx = 0;

	public:
		inline allocator() :
			m_nodes(k_max_allocable_nodes)
		{
			VREN_INFO("vren::render_graph::allocator | Allocating {} bytes for render-graph\n", m_nodes.size() * sizeof(vren::render_graph::node));
		}

		inline vren::render_graph::node* allocate()
		{
			assert(m_next_node_idx < k_max_allocable_nodes);

			auto& node = m_nodes.at(m_next_node_idx);
			node.m_idx = m_next_node_idx;
			m_next_node_idx++;
			return &node;
		}

		inline vren::render_graph::node* get_node_at(vren::render_graph::node_index_t node_idx)
		{
			return &m_nodes.at(node_idx);
		}

		inline void clear()
		{
			m_next_node_idx = 0;
		}
	};
}

// ------------------------------------------------------------------------------------------------

namespace vren
{
	vren::render_graph::node* clear_color_buffer(vren::render_graph::allocator& allocator, VkImage color_buffer, VkClearColorValue clear_color_value);
	vren::render_graph::node* clear_depth_stencil_buffer(vren::render_graph::allocator& allocator, VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value);
}
