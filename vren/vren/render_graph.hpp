#pragma once

#include <cassert>
#include <cstring>
#include <vector>
#include <functional>
#include <type_traits>
#include <filesystem>

#include <volk.h>

#include "base/base.hpp"
#include "base/resource_container.hpp"
#include "utils/log.hpp"

namespace vren::render_graph
{
	// Forward decl
	class allocator;

	// ------------------------------------------------------------------------------------------------

	inline constexpr size_t k_max_name_length = 64;

	using node_idx_t = uint8_t;

	// ------------------------------------------------------------------------------------------------
	// Image access
	// ------------------------------------------------------------------------------------------------

	struct image_access
	{
		static constexpr size_t k_max_name_length = vren::render_graph::k_max_name_length;

		char const* m_name = "unnamed";

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

		char const* m_name = "unnamed";

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

		using image_access_idx_t = uint8_t;
		using buffer_access_idx_t = uint8_t;

		using callback_t = std::function<void(uint32_t frame_idx, VkCommandBuffer command_buffer, resource_container& resource_container)>;

		static_assert(std::numeric_limits<buffer_access_idx_t>::max() >= k_max_buffer_accesses - 1);
		static_assert(std::numeric_limits<node_idx_t>::max() >= k_max_previous_nodes - 1);
		static_assert(std::numeric_limits<node_idx_t>::max() >= k_max_next_nodes - 1);

	private:
		allocator* m_allocator;
		node_idx_t m_allocation_idx;

		char const* m_name = "unnamed";

		VkPipelineStageFlags m_src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags m_dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		static_vector_t<image_access, k_max_image_accesses> m_image_accesses;
		static_vector_t<buffer_access, k_max_buffer_accesses> m_buffer_accesses;

		callback_t m_callback;

		static_vector_t<node_idx_t, k_max_previous_nodes> m_previous_nodes;
		static_vector_t<node_idx_t, k_max_next_nodes> m_next_nodes;

	public:
		node(allocator& allocator) :
			m_allocator(&allocator)
		{
		}

		inline auto* get_allocator()
		{
			return m_allocator;
		}

		inline auto get_idx() const
		{
			return m_allocation_idx;
		}

		inline auto get_name() const
		{
			return m_name;
		}

		inline void set_name(char const* name)
		{
			m_name = name;
		}

		inline auto get_src_stage() const
		{
			return m_src_stage;
		}

		inline void set_src_stage(VkPipelineStageFlags stage)
		{
			m_src_stage = stage;
		}

		inline auto get_dst_stage() const
		{
			return m_dst_stage;
		}

		inline void set_dst_stage(VkPipelineStageFlags stage)
		{
			m_dst_stage = stage;
		}

		inline void add_image(image_access const& image_access)
		{
			m_image_accesses.push_back(image_access);
		}

		inline auto const& get_image_accesses() const
		{
			return m_image_accesses;
		}

		inline void add_buffer(buffer_access const& buffer_access)
		{
			m_buffer_accesses.push_back(buffer_access);
		}

		inline auto const& get_buffer_accesses() const
		{
			return m_buffer_accesses;
		}

		inline void operator()(uint32_t frame_idx, VkCommandBuffer command_buffer, resource_container& resource_container) const
		{
			if (m_callback) {
				m_callback(frame_idx, command_buffer, resource_container);
			}
		}

		inline void set_callback(callback_t const& callback)
		{
			m_callback = callback;
		}

		inline auto const& get_previous_nodes() const
		{
			return m_previous_nodes;
		}

		inline auto const& get_next_nodes() const
		{
			return m_next_nodes;
		}

		inline void add_next(node* next_node)
		{
			assert(next_node != nullptr);
			assert(this != next_node);
			assert(!next_node->m_previous_nodes.full());
			assert(!m_next_nodes.full());

			m_next_nodes.push_back(next_node->m_allocation_idx);
			next_node->m_previous_nodes.push_back(m_allocation_idx);
		}

		// Forward decl (need allocator)
		void add_next(node_idx_t next_node_idx);
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph allocator
	// ------------------------------------------------------------------------------------------------

	class allocator
	{
	public:
		static constexpr size_t k_max_allocable_nodes = 256;

		static_assert(std::numeric_limits<node_idx_t>::max() >= k_max_allocable_nodes - 1);

	private:
		std::vector<node> m_nodes;

	public:
		inline allocator()
		{
			m_nodes.reserve(k_max_allocable_nodes);

			VREN_INFO("[render_graph::allocator] Max allocable nodes {} ({} bytes)\n", k_max_allocable_nodes, k_max_allocable_nodes * sizeof(vren::render_graph::node));
		}

		inline node* allocate()
		{
			auto& node = m_nodes.emplace_back(*this);
			node.m_allocation_idx = m_nodes.size() - 1;
			return &node;
		}

		inline node* get_node_at(node_idx_t node_idx)
		{
			return &m_nodes.at(node_idx);
		}

		inline void clear()
		{
			m_nodes.clear();
		}
	};

	// ------------------------------------------------------------------------------------------------

	inline void vren::render_graph::node::add_next(node_idx_t next_node_idx)
	{
		add_next(m_allocator->get_node_at(next_node_idx));
	}

	// ------------------------------------------------------------------------------------------------
	// Render-graph
	// ------------------------------------------------------------------------------------------------

	using graph_t = static_vector_t<node_idx_t, allocator::k_max_allocable_nodes>;

	using traverse_callback_t = std::function<bool(node_idx_t node_idx)>;
	void traverse(
		allocator& allocator,
		graph_t const& graph,
		traverse_callback_t const& callback, // The function called every time a node is reached
		bool forward_traversal = true,       // Whether the traversal should go forward or backward
		bool callback_initial_nodes = true   // Whether the callback should be called for graph initial nodes
	);

	graph_t get_starting_nodes(allocator& allocator, graph_t const& graph);
	graph_t get_ending_nodes(allocator& allocator, graph_t const& graph);

	template<typename... _nodes> requires (std::is_same_v<node, typename std::remove_pointer<_nodes>::type> && ...)
	graph_t gather(_nodes... nodes)
	{
		return { (nodes->get_idx(), ...) };
	}

	graph_t concat(allocator& allocator, graph_t const& left, graph_t const& right);

	// ------------------------------------------------------------------------------------------------
	// Render-graph chain (builder)
	// ------------------------------------------------------------------------------------------------

	class chain
	{
	private:
		allocator* m_allocator;

	public:
		graph_t m_head;
		graph_t m_tail;

		inline chain(allocator& allocator) :
			m_allocator(&allocator)
		{
		}

		inline void concat(graph_t const& graph)
		{
			if (m_head.empty())
			{
				m_head = graph;
				m_tail = graph;
			}
			else
			{
				m_tail = vren::render_graph::concat(*m_allocator, m_tail, graph);
			}
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph execution
	// ------------------------------------------------------------------------------------------------

	namespace detail
	{
		template<
		    typename _execution_parameters,
			typename _make_initial_image_layout_transition,
			typename _execute_node,
			typename _put_image_memory_barrier,
			typename _put_buffer_memory_barrier
		>
		void execute(
			vren::render_graph::allocator& allocator,
			vren::render_graph::graph_t const& graph,
			_execution_parameters& params
		);
	}

	void execute(
		vren::render_graph::allocator& allocator,
		vren::render_graph::graph_t const& graph,
		uint32_t frame_idx,
		VkCommandBuffer command_buffer,
		vren::resource_container& resource_container
	);

	void dump(
		vren::render_graph::allocator& allocator,
		vren::render_graph::graph_t const& graph,
		std::ostream& output
	);
}

// ------------------------------------------------------------------------------------------------

namespace vren
{
	render_graph::graph_t clear_color_buffer(vren::render_graph::allocator& allocator, VkImage color_buffer, VkClearColorValue clear_color_value);
	render_graph::graph_t clear_depth_stencil_buffer(vren::render_graph::allocator& allocator, VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value);
}
