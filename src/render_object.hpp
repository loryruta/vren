#pragma once

#include <memory>
#include <vector>
#include <array>
#include <stdexcept>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "render_list.hpp"
#include "utils/image.hpp"
#include "utils/buffer.hpp"
#include "material.hpp"

namespace vren
{
	// --------------------------------------------------------------------------------------------------------------------------------

	struct vertex
	{
		glm::vec3 m_position;
		glm::vec3 m_normal;
		glm::vec3 m_tangent;
		glm::vec2 m_texcoord;
		glm::vec4 m_color;
	};

	using vertex_index_t = uint32_t; // todo use it

	struct instance_data
	{
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

	class render_object
	{
	public:
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
				.offset = offsetof(vren::vertex, m_position)
			};

			// Normal
			attrib_desc[1] = {
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(vren::vertex, m_normal)
			};

			// Tangent
			attrib_desc[2] = {
				.location = 2,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = offsetof(vren::vertex, m_tangent)
			};

			// Texcoord
			attrib_desc[3] = {
				.location = 3,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(vren::vertex, m_texcoord)
			};

			// Color
			attrib_desc[4] = {
				.location = 4,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32A32_SFLOAT,
				.offset = offsetof(vren::vertex, m_color)
			};

			return attrib_desc;
		}

		static constexpr auto get_idb_binding_desc()
		{
			std::array<VkVertexInputBindingDescription, 1> binding_desc{};
			binding_desc[0] = {
				.binding = 1,
				.stride = sizeof(vren::instance_data),
				.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
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

		static constexpr VkIndexType s_index_type = VK_INDEX_TYPE_UINT32;

	public:
		std::shared_ptr<vren::render_list> m_render_list;

		uint32_t m_idx = -1;

		std::shared_ptr<vren::vk_utils::buffer> m_vertices_buffer;
		uint32_t m_vertices_count;

		std::shared_ptr<vren::vk_utils::buffer> m_indices_buffer;
		uint32_t m_indices_count;

		std::shared_ptr<vren::vk_utils::buffer> m_instances_buffer;
		uint32_t m_instances_count;

		std::shared_ptr<vren::material> m_material;

		explicit render_object(std::shared_ptr<vren::render_list> const& render_list);
		render_object(vren::render_object const& other) = delete;
		render_object(vren::render_object&& other) noexcept;

		vren::render_object& operator=(vren::render_object const& other) = delete;
		vren::render_object& operator=(vren::render_object&& other) noexcept;

		bool is_valid() const;

		void set_vertices_buffer(std::shared_ptr<vren::vk_utils::buffer> const& vertices_buf, size_t vertices_count);
		void set_indices_buffer(std::shared_ptr<vren::vk_utils::buffer> const& indices_buf, size_t indices_count);
		void set_instances_buffer(std::shared_ptr<vren::vk_utils::buffer> const& instances_buf, size_t instances_count);
	};
}
