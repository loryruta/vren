#pragma once

#include <cassert>
#include <cstring>
#include <vector>
#include <functional>
#include <type_traits>
#include <filesystem>
#include <unordered_set>

#include "volk.h"

#include "base/base.hpp"
#include "base/resource_container.hpp"

namespace vren
{
	// Forward decl
	class render_graph_allocator;

	// ------------------------------------------------------------------------------------------------
	// Render-graph image resource
	// ------------------------------------------------------------------------------------------------

	struct render_graph_image_info
	{
		char const* m_name = "unnamed";

		VkImage m_image = VK_NULL_HANDLE;
		VkImageAspectFlags m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		uint32_t m_mip_level = 0;
		uint32_t m_layer = 0;

		inline bool operator==(vren::render_graph_image_info const& other) const
		{
			return m_image == other.m_image && m_image_aspect == other.m_image_aspect && m_mip_level == other.m_mip_level && m_layer == other.m_layer;
		}
	};

	struct render_graph_image_info_hash
	{
		size_t operator()(vren::render_graph_image_info const& element) const noexcept
		{
			return size_t(((uint64_t) element.m_image) ^ element.m_image_aspect ^ ((uint64_t(element.m_mip_level) << 32) | element.m_layer));
		};
	};

	struct render_graph_node_image_access
	{
		uint32_t m_image_idx;
		VkImageLayout m_image_layout;
		VkAccessFlags m_access_flags;
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph buffer resource
	// ------------------------------------------------------------------------------------------------

	struct render_graph_buffer_info
	{
		char const* m_name = "unnamed";

		VkBuffer m_buffer = VK_NULL_HANDLE;

		inline bool operator==(vren::render_graph_buffer_info const& other) const
		{
			return m_buffer == other.m_buffer;
		}
	};

	struct render_graph_buffer_info_hash
	{
		size_t operator()(vren::render_graph_buffer_info const& element) const noexcept
		{
			return (size_t) element.m_buffer;
		};
	};

	struct render_graph_node_buffer_access
	{
		uint32_t m_buffer_idx;
		VkAccessFlags m_access_flags;
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph node
	// ------------------------------------------------------------------------------------------------

	using render_graph_node_index_t = uint8_t;

	class render_graph_node
	{
		friend vren::render_graph_allocator;

	public:
		static const size_t k_max_image_accesses = 32;
		static const size_t k_max_buffer_accesses = 32;
		static const size_t k_max_previous_nodes = 4;
		static const size_t k_max_next_nodes = 4;

		using callback_t = std::function<void(uint32_t frame_idx, VkCommandBuffer command_buffer, resource_container& resource_container)>;

		using image_access_index_t = uint16_t;
		using buffer_access_index_t = uint16_t;

		static_assert(std::numeric_limits<image_access_index_t>::max() >= k_max_image_accesses - 1);
		static_assert(std::numeric_limits<buffer_access_index_t>::max() >= k_max_buffer_accesses - 1);
		static_assert(std::numeric_limits<vren::render_graph_node_index_t>::max() >= k_max_previous_nodes - 1);
		static_assert(std::numeric_limits<vren::render_graph_node_index_t>::max() >= k_max_next_nodes - 1);

	private:
		vren::render_graph_allocator* m_allocator;
		vren::render_graph_node_index_t m_allocation_idx;

		char const* m_name = "unnamed";

		VkPipelineStageFlags m_src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkPipelineStageFlags m_dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		vren::static_vector_t<vren::render_graph_node_image_access, k_max_image_accesses> m_image_accesses;
		vren::static_vector_t<vren::render_graph_node_buffer_access, k_max_buffer_accesses> m_buffer_accesses;

		callback_t m_callback;

		vren::static_vector_t<vren::render_graph_node_index_t, k_max_previous_nodes> m_previous_nodes;
		vren::static_vector_t<vren::render_graph_node_index_t, k_max_next_nodes> m_next_nodes;

	public:
		render_graph_node(vren::render_graph_allocator& allocator) :
			m_allocator(&allocator)
		{
		}

		inline vren::render_graph_allocator* get_allocator() const
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

		inline auto const& get_image_accesses() const
		{
			return m_image_accesses;
		}

		inline auto const& get_buffer_accesses() const
		{
			return m_buffer_accesses;
		}

		inline void operator()(uint32_t frame_idx, VkCommandBuffer command_buffer, resource_container& resource_container) const
		{
			if (m_callback)
			{
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

		inline void add_next(vren::render_graph_node* next_node)
		{
			assert(next_node != nullptr);
			assert(this != next_node);
			assert(!next_node->m_previous_nodes.full());
			assert(!m_next_nodes.full());

			m_next_nodes.push_back(next_node->m_allocation_idx);
			next_node->m_previous_nodes.push_back(m_allocation_idx);
		}

		// Forward decl
		void add_image(vren::render_graph_image_info const& image_info, VkImageLayout image_layout, VkAccessFlags access_flags);
		void add_buffer(vren::render_graph_buffer_info const& buffer_info, VkAccessFlags access_flags);
		void add_next(vren::render_graph_node_index_t next_node_idx);
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph allocator
	// ------------------------------------------------------------------------------------------------

	class render_graph_allocator
	{
	public:
		static const size_t k_max_allocable_nodes = 256;
		static const size_t k_max_image_infos = 1024;
		static const size_t k_max_buffer_infos = 1024;

		static_assert(std::numeric_limits<vren::render_graph_node_index_t>::max() >= k_max_allocable_nodes - 1);

	private:
		std::vector<vren::render_graph_node> m_nodes;
		
		std::vector<vren::render_graph_image_info> m_image_infos;
		std::vector<vren::render_graph_buffer_info> m_buffer_infos;

	public:
		inline render_graph_allocator()
		{
			m_nodes.reserve(k_max_allocable_nodes);
			m_image_infos.reserve(k_max_image_infos);
			m_buffer_infos.reserve(k_max_buffer_infos);
		}

		inline vren::render_graph_node* allocate()
		{
			assert(m_nodes.size() <= std::numeric_limits<vren::render_graph_node_index_t>::max() + 1);

			vren::render_graph_node& node = m_nodes.emplace_back(*this);
			node.m_allocation_idx = m_nodes.size() - 1;
			return &node;
		}

		inline vren::render_graph_node* get_node_at(vren::render_graph_node_index_t node_idx)
		{
			return &m_nodes.at(node_idx);
		}

		inline uint32_t subscribe_image_info(vren::render_graph_image_info const& image_info)
		{
			for (uint32_t i = 0; i < m_image_infos.size(); i++)
			{
				if (m_image_infos.at(i) == image_info)
				{
					return i;
				}
			}

			assert(m_image_infos.size() < k_max_image_infos);

			m_image_infos.emplace_back(image_info);
			return m_image_infos.size() - 1;
		}

		inline vren::render_graph_image_info const& get_image_info_at(uint32_t index)
		{
			return m_image_infos.at(index);
		}

		inline uint32_t subscribe_buffer_info(vren::render_graph_buffer_info const& buffer_info)
		{
			for (uint32_t i = 0; i < m_buffer_infos.size(); i++)
			{
				if (m_buffer_infos.at(i) == buffer_info)
				{
					return i;
				}
			}

			assert(m_buffer_infos.size() < k_max_buffer_infos);

			m_buffer_infos.emplace_back(buffer_info);
			return m_buffer_infos.size() - 1;
		}

		inline vren::render_graph_buffer_info const& get_buffer_info_at(uint32_t index)
		{
			return m_buffer_infos.at(index);
		}

		inline void clear()
		{
			m_nodes.clear();
			m_image_infos.clear();
			m_buffer_infos.clear();
		}
	};

	// ------------------------------------------------------------------------------------------------

	inline void vren::render_graph_node::add_image(vren::render_graph_image_info const& image_info, VkImageLayout image_layout, VkAccessFlags access_flags)
	{
		for (uint32_t i = 0; i < sizeof(image_info.m_image_aspect) * 8; i++)
		{
			if ((image_info.m_image_aspect & (1 << i)) != 0)
			{
				vren::render_graph_image_info single_image_aspect = image_info;
				single_image_aspect.m_image_aspect &= (1 << i);

				m_image_accesses.push_back(vren::render_graph_node_image_access{
					.m_image_idx = m_allocator->subscribe_image_info(single_image_aspect),
					.m_image_layout = image_layout,
					.m_access_flags = access_flags
				});
			}
		}

	}

	inline void vren::render_graph_node::add_buffer(vren::render_graph_buffer_info const& buffer_info, VkAccessFlags access_flags)
	{
		m_buffer_accesses.push_back(vren::render_graph_node_buffer_access{
			.m_buffer_idx = m_allocator->subscribe_buffer_info(buffer_info),
			.m_access_flags = access_flags
		});
	}

	inline void vren::render_graph_node::add_next(vren::render_graph_node_index_t next_node_idx)
	{
		add_next(m_allocator->get_node_at(next_node_idx));
	}

	// ------------------------------------------------------------------------------------------------
	// Render-graph
	// ------------------------------------------------------------------------------------------------

	using render_graph_t = vren::static_vector_t<vren::render_graph_node_index_t, vren::render_graph_allocator::k_max_allocable_nodes>;

	using render_graph_traverse_callback_t = std::function<bool(vren::render_graph_node_index_t node_idx)>;

	void render_graph_traverse(
		vren::render_graph_allocator& allocator,
		vren::render_graph_t const& graph,
		vren::render_graph_traverse_callback_t const& callback,
		bool forward_traversal = true,
		bool callback_initial_nodes = true
	);

	vren::render_graph_t render_graph_get_start(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph);
	vren::render_graph_t render_graph_get_end(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph);

	template<typename... _nodes> requires (std::is_same_v<vren::render_graph_node, typename std::remove_pointer<_nodes>::type> && ...)
	vren::render_graph_t render_graph_gather(_nodes... nodes)
	{
		return { (nodes->get_idx(), ...) };
	}

	vren::render_graph_t render_graph_concat(vren::render_graph_allocator& allocator, vren::render_graph_t const& left, vren::render_graph_t const& right);

	// ------------------------------------------------------------------------------------------------
	// Render-graph builder
	// ------------------------------------------------------------------------------------------------
	
	class render_graph_builder
	{
	private:
		vren::render_graph_allocator* m_allocator;

		vren::render_graph_t m_head;
		vren::render_graph_t m_tail;

	public:
		inline render_graph_builder(vren::render_graph_allocator& allocator) :
			m_allocator(&allocator)
		{
		}

		inline vren::render_graph_t const& get_head() const
		{
			return m_head;
		}

		inline vren::render_graph_t const& get_tail() const
		{
			return m_tail;
		}

		inline void concat(vren::render_graph_t const& graph)
		{
			if (m_head.empty())
			{
				m_head = graph;
				m_tail = graph;
			}
			else
			{
				m_tail = vren::render_graph_concat(*m_allocator, m_tail, graph);
			}
		}
	};

	// ------------------------------------------------------------------------------------------------
	// Render-graph execution
	// ------------------------------------------------------------------------------------------------

	namespace detail
	{
		class render_graph_executor
		{
		protected:
			virtual void make_initial_image_layout_transition(vren::render_graph_node const& node, vren::render_graph_node_image_access const& image_access) = 0;

			virtual void execute_node(vren::render_graph_node const& node) = 0;

			virtual void place_image_memory_barrier(
				vren::render_graph_node const& node_1,
				vren::render_graph_node const& node_2,
				vren::render_graph_node_image_access const& image_access_1,
				vren::render_graph_node_image_access const& image_access_2
			) = 0;

			virtual void place_buffer_memory_barrier(
				vren::render_graph_node const& node_1,
				vren::render_graph_node const& node_2,
				vren::render_graph_node_buffer_access const& buffer_access_1,
				vren::render_graph_node_buffer_access const& buffer_access_2
			) = 0;

		public:
			void execute(
				vren::render_graph_allocator& allocator,
				vren::render_graph_t const& graph
			);
		};
	}

	class render_graph_executor : public vren::detail::render_graph_executor
	{
	protected:
		void make_initial_image_layout_transition(vren::render_graph_node const& node, vren::render_graph_node_image_access const& image_access) override;

		void execute_node(vren::render_graph_node const& node) override;

		void place_image_memory_barrier(
			vren::render_graph_node const& node_1,
			vren::render_graph_node const& node_2,
			vren::render_graph_node_image_access const& image_access_1,
			vren::render_graph_node_image_access const& image_access_2
		) override;

		void place_buffer_memory_barrier(
			vren::render_graph_node const& node_1,
			vren::render_graph_node const& node_2,
			vren::render_graph_node_buffer_access const& buffer_access_1,
			vren::render_graph_node_buffer_access const& buffer_access_2
		) override;

	private:
		uint32_t m_frame_idx;
		VkCommandBuffer m_command_buffer;
		vren::resource_container* m_resource_container;

	public:
		render_graph_executor(uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container);
		
		void execute(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph);
	};

	class render_graph_dumper : public vren::detail::render_graph_executor
	{
	protected:
		void make_initial_image_layout_transition(vren::render_graph_node const& node, vren::render_graph_node_image_access const& image_access) override;

		void execute_node(vren::render_graph_node const& node) override;

		void place_image_memory_barrier(
			vren::render_graph_node const& node_1,
			vren::render_graph_node const& node_2,
			vren::render_graph_node_image_access const& image_access_1,
			vren::render_graph_node_image_access const& image_access_2
		) override;

		void place_buffer_memory_barrier(
			vren::render_graph_node const& node_1,
			vren::render_graph_node const& node_2,
			vren::render_graph_node_buffer_access const& buffer_access_1,
			vren::render_graph_node_buffer_access const& buffer_access_2
		) override;

	private:
		std::ostream* m_output;
		uint32_t m_execution_order = 0;

	public:
		render_graph_dumper(std::ostream& output);

		void dump(vren::render_graph_allocator& allocator, vren::render_graph_t const& graph);
	};
}

// ------------------------------------------------------------------------------------------------

namespace vren
{
	vren::render_graph_t clear_color_buffer(vren::render_graph_allocator& allocator, VkImage color_buffer, VkClearColorValue clear_color_value);
	vren::render_graph_t clear_depth_stencil_buffer(vren::render_graph_allocator& allocator, VkImage depth_buffer, VkClearDepthStencilValue clear_depth_stencil_value);
}
