#pragma once

#include <vector>
#include <array>
#include <stdexcept>

#include "gpu_allocator.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace vren
{
	// Forward decl

	class renderer;

	// --------------------------------------------------------------------------------------------------------------------------------

	struct vertex {
		glm::vec3 m_position;
		glm::vec3 m_normal;
		glm::vec4 m_tangent;
		glm::vec2 m_texcoord;
		glm::vec4 m_color;
	};

	struct instance_data {
		glm::mat4 m_transform;
	};

	// -------------------------------------------------------------------------------------------------------------------------------- render_object

	template<typename T, std::size_t N, std::size_t M, std::size_t...IsN, std::size_t...IsM>
	constexpr std::array<T, N+M> join_impl( std::array<T, N> const & a, std::array<T, M> const & b, std::index_sequence<IsN...>, std::index_sequence<IsM...> ) {
		return std::array<T, N+M>{ a[IsN]..., b[IsM]... };
	}

	template<typename T, std::size_t N, std::size_t M>
	constexpr std::array<T, N+M> join( std::array<T, N> const & a, std::array<T, M> const & b ) {
		return join_impl( a, b, std::make_index_sequence<N>{}, std::make_index_sequence<M>{} );
	}

	struct render_object
	{
		static constexpr auto get_vbo_binding_desc()
		{
			std::array<VkVertexInputBindingDescription, 1> binding_desc{};
			binding_desc[0] = {
				.binding = 0,
				.stride = sizeof(vren::vertex),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
			return binding_desc;
		}

		static constexpr auto get_vbo_attrib_desc()
		{
			std::array<VkVertexInputAttributeDescription, 5> attrib_desc{};

			// Position
			attrib_desc[0] = {
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = 0
			};

			// Normal
			attrib_desc[1] = {
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = sizeof(glm::vec3)
			};

			// Tangent
			attrib_desc[2] = {
				.location = 2,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = sizeof(glm::vec3) + sizeof(glm::vec3)
			};

			// Texcoord
			attrib_desc[3] = {
				.location = 3,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec4)
			};

			// Color
			attrib_desc[4] = {
				.location = 4,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec4) + sizeof(glm::vec2)
			};

			return attrib_desc;
		}

		static constexpr auto get_idb_binding_desc()
		{
			std::array<VkVertexInputBindingDescription, 1> binding_desc{};
			binding_desc[0] = {
				.binding = 1,
				.stride = sizeof(vren::instance_data),
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
			return binding_desc;
		}

		static constexpr auto get_idb_attrib_desc()
		{
			std::array<VkVertexInputAttributeDescription, 4> attrib_desc{};

			// Transform 0
			attrib_desc[0] = {
				.location = 16,
				.binding = 1,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = 0
			};

			// Transform 1
			attrib_desc[1] = {
				.location = 17,
				.binding = 1,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = sizeof(glm::vec4)
			};

			// Transform 2
			attrib_desc[2] = {
				.location = 18,
				.binding = 1,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = sizeof(glm::vec4) * 2
			};

			// Transform 3
			attrib_desc[3] = {
				.location = 19,
				.binding = 1,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = sizeof(glm::vec4) * 3
			};

			return attrib_desc;
		}
		
		static constexpr auto get_all_binding_desc()
		{
			return vren::join(get_vbo_binding_desc(), get_idb_binding_desc());
		}

		static constexpr auto get_all_attrib_desc()
		{
			return vren::join(get_vbo_attrib_desc(), get_idb_attrib_desc());
		}

		vren::renderer* m_renderer;

		uint32_t m_render_list_idx = -1;

		std::vector<vren::gpu_buffer> m_vertex_buffers;

		vren::gpu_buffer m_indices_buffer;
		uint32_t m_indices_count;
		static constexpr VkIndexType s_index_type = VK_INDEX_TYPE_UINT32;

		std::vector<vren::gpu_buffer> m_instances_buffers;
		uint32_t m_instances_count;

		render_object(vren::renderer* renderer);
		~render_object();

		void set_vertex_data(vren::vertex const* vertices, size_t count);
		void set_indices_data(uint32_t const* indices, size_t count);
		void set_instances_data(vren::instance_data const* instances_data, size_t count);
	};
}
