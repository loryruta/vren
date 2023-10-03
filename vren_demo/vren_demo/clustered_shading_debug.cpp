#include "clustered_shading_debug.hpp"

#include <vren/Toolbox.hpp>
#include <vren/vk_helpers/misc.hpp>

vren_demo::show_clusters::show_clusters(
    vren::context const& context
) :
    m_context(&context),
	m_pipeline([&]()
	{
		vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/show_clusters.comp.spv");
		vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
		return vren::create_compute_pipeline(*m_context, shader);
	}())
{
}

vren::render_graph_t vren_demo::show_clusters::operator()(
	vren::render_graph_allocator& render_graph_allocator,
	glm::uvec2 const& screen,
	int32_t mode,
	vren::cluster_and_shade const& cluster_and_shade,
	vren::vk_utils::combined_image_view const& output
)
{
	vren::render_graph_node* node = render_graph_allocator.allocate();

	node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	// cluster_reference_buffer isn't part of the render_graph (it's an internal buffer), we put a generic barrier before using it
	// Same for assigned_light_buffer
	node->add_image({ .m_image = output.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = 0, .m_layer = 0, }, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT);

	node->set_callback([
		this,
		screen,
		mode,
		&cluster_and_shade,
		&output
	](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkMemoryBarrier memory_barrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		};
		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			NULL,
			1, &memory_barrier,
			0, nullptr,
			0, nullptr
		);

		m_pipeline.bind(command_buffer);

		struct
		{
			int32_t m_mode;
			glm::uvec2 m_num_tiles;
			float _pad;
		} push_constants;

		push_constants = {
			.m_mode = mode,
			.m_num_tiles = glm::uvec2(
				vren::divide_and_ceil(screen.x, 32u),
				vren::divide_and_ceil(screen.y, 32u)
			),
		};

		m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants));

		auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
		);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 0, cluster_and_shade.m_cluster_reference_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 1, cluster_and_shade.m_cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 2, cluster_and_shade.m_assigned_light_counts_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 3, cluster_and_shade.m_assigned_light_offsets_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 4, cluster_and_shade.m_assigned_light_indices_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 5, output.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);

		m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);

		uint32_t num_workgroups_x = vren::divide_and_ceil(screen.x, 32);
		uint32_t num_workgroups_y = vren::divide_and_ceil(screen.y, 32);
		m_pipeline.dispatch(command_buffer, num_workgroups_x, num_workgroups_y, 1);

		resource_container.add_resources(
			descriptor_set_0
		);
	});

	return vren::render_graph_gather(node);
}

vren_demo::show_clusters_geometry::show_clusters_geometry(
	vren::context const& context
) :
	m_context(&context),
	m_pipeline([&]()
	{
		vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/show_clusters_geometry.comp.spv");
		vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
		return vren::create_compute_pipeline(*m_context, shader);
	}())
{
}

vren::debug_renderer_draw_buffer vren_demo::show_clusters_geometry::operator()(
	vren::context const& context,
	vren::cluster_and_shade const& cluster_and_shade,
	vren::Camera const& camera,
	glm::vec2 const& screen
)
{
	vren::debug_renderer_draw_buffer debug_draw_buffer(context);
	
	// Retrieve the number of cluster keys from the indirect command's buffer
	vren::vk_utils::buffer staging_buffer = vren::vk_utils::alloc_host_only_buffer(context, VK_BUFFER_USAGE_TRANSFER_DST_BIT, sizeof(glm::uvec4), true);
	vren::vk_utils::immediate_graphics_queue_submit(context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, NULL, 0, nullptr, 0, nullptr, 0, nullptr);

		VkBufferCopy region{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = sizeof(glm::uvec4),
		};
		vkCmdCopyBuffer(command_buffer, cluster_and_shade.m_cluster_key_dispatch_params_buffer.m_buffer.m_handle, staging_buffer.m_buffer.m_handle, 1, &region);

		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 0, nullptr, 0, nullptr);
	});

	glm::uvec4* dispatch_params = reinterpret_cast<glm::uvec4*>(staging_buffer.get_mapped_pointer());

	// Allocate the debug draw buffer
	const uint32_t k_line_vertices = 2;
	const uint32_t k_arrow_vertices = k_line_vertices + k_line_vertices * 3;
	const uint32_t k_cube_vertices = 12 * k_line_vertices;

	uint32_t vertex_count = dispatch_params->w * (k_cube_vertices + k_arrow_vertices);
	debug_draw_buffer.m_vertex_buffer.set_data(nullptr, vertex_count);
	debug_draw_buffer.m_vertex_count = vertex_count;

	// Finally fill the debug draw buffer with clusters' geometry
	vren::vk_utils::immediate_graphics_queue_submit(context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 0, nullptr);

		// Bind pipeline
		m_pipeline.bind(command_buffer);

		// Push constants
		struct {
			glm::uvec2 m_num_tiles;
			float m_camera_near;
			float m_camera_half_fov;
			glm::mat4 m_camera_proj;
			glm::mat4 m_camera_view;
		} push_constants;

		push_constants = {
			.m_num_tiles = glm::uvec2(
				vren::divide_and_ceil(screen.x, 32u),
				vren::divide_and_ceil(screen.y, 32u)
			),
			.m_camera_near = camera.m_near_plane,
			.m_camera_half_fov = camera.m_fov_y / 2.0f,
			.m_camera_proj = camera.get_projection(),
			.m_camera_view = camera.get_view(),
		};

		m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants), 0);

		// Descriptor set 0
		auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
		);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 0, cluster_and_shade.m_cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 1, cluster_and_shade.m_cluster_key_dispatch_params_buffer.m_buffer.m_handle, sizeof(glm::uvec4), 0);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 2, debug_draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle, VK_WHOLE_SIZE, 0);

		m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set_0->m_handle.m_descriptor_set);
		
		// Dispatch
		m_pipeline.dispatch(command_buffer, dispatch_params->x, dispatch_params->y, dispatch_params->z);

		resource_container.add_resources(
			descriptor_set_0
		);

		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, NULL, 0, nullptr, 0, nullptr, 0, nullptr);
	});

	return debug_draw_buffer;
}


vren::debug_renderer_draw_buffer vren_demo::debug_camera_clusters_geometry(
	vren::context const& context,
	vren::Camera const& camera,
	glm::vec2 const& screen,
	vren::cluster_and_shade const& cluster_and_shade
)
{
	const uint32_t k_camera_frustum_color = 0xffffff;
	const uint32_t k_camera_plane_color = 0xffffff;

	glm::uvec2 num_tiles = glm::uvec2(glm::ceil(screen / glm::vec2(32)));

	glm::mat4 inv_view = glm::inverse(camera.get_view());
	glm::mat4 inv_proj = glm::inverse(camera.get_projection());

	auto to_world_space = [&](glm::vec3 const& ndc_position)
	{
		glm::vec4 tmp = inv_proj * glm::vec4(ndc_position, 1.0f);
		tmp /= tmp.w;
		return inv_view * tmp;
	};

	float tan_half_fov = glm::tan(camera.m_fov_y / 2.0f);

	vren::debug_renderer_draw_buffer draw_buffer(context);

	// Frustum border
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(1.0f, 1.0f, 1.0f)), .m_color = k_camera_frustum_color, });
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(-1.0f, 1.0f, 1.0f)), .m_color = k_camera_frustum_color, });
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(1.0f, -1.0f, 1.0f)), .m_color = k_camera_frustum_color, });
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(-1.0f, -1.0f, 1.0f)), .m_color = k_camera_frustum_color, });

	// Near plane
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, 1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(1.0f, -1.0f, 0.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, -1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(-1.0f, -1.0f, 0.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, -1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(-1.0f, 1.0f, 0.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, 1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(1.0f, 1.0f, 0.0f)), .m_color = k_camera_plane_color, });

	// Far plane
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, 1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(1.0f, -1.0f, 1.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, -1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(-1.0f, -1.0f, 1.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, -1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(-1.0f, 1.0f, 1.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, 1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(1.0f, 1.0f, 1.0f)), .m_color = k_camera_plane_color, });

	auto write_cluster = [&](glm::uvec3 const& cluster_ijk, uint32_t color)
	{
		glm::vec2 tile_min = glm::vec2(cluster_ijk.x, cluster_ijk.y);
		tile_min /= num_tiles;
		tile_min.y = (1.0 - tile_min.y) * 2.0 - 1.0;
		tile_min.x = tile_min.x * 2.0 - 1.0;

		glm::vec2 tile_max = glm::vec2(cluster_ijk.x, cluster_ijk.y) + glm::vec2(1.0f);
		tile_max /= num_tiles;
		tile_max.y = (1.0 - tile_max.y) * 2.0 - 1.0;
		tile_max.x = tile_max.x * 2.0 - 1.0;

		glm::vec2 tmp = tile_min;
		tile_min = glm::min(tile_min, tile_max);
		tile_max = glm::max(tile_max, tmp);

		float near_k = camera.m_near_plane * glm::pow(1.0f + ((2.0f * tan_half_fov) / float(num_tiles.y)), float(cluster_ijk.z));
		float h_k = (tile_max.y * tan_half_fov * near_k) - (tile_min.y * tan_half_fov * near_k);
		float far_k = near_k + h_k;

		glm::vec4 cluster_min{};
		cluster_min.x = tile_min.x * tan_half_fov * camera.m_aspect_ratio * near_k;
		cluster_min.y = tile_min.y * tan_half_fov * near_k;
		cluster_min.z = near_k;
		cluster_min.w = 1.0;

		glm::vec4 cluster_max{};
		cluster_max.x = cluster_min.x + h_k;
		cluster_max.y = cluster_min.y + h_k;
		cluster_max.z = near_k + h_k; // far_k
		cluster_max.w = 1.0;

		cluster_min = inv_view * cluster_min;
		cluster_max = inv_view * cluster_max;

		glm::vec4 tmp_1 = cluster_min;
		cluster_min = glm::min(cluster_min, cluster_max);
		cluster_max = glm::max(tmp_1, cluster_max);

		draw_buffer.add_cube({ .m_min = cluster_min, .m_max = cluster_max, .m_color = color, });
	};

	cluster_and_shade.m_cluster_key_buffer;

	glm::uvec3 cluster_ijk{};
	uint32_t cluster_max_depth = 100;

	write_cluster(cluster_ijk, (cluster_ijk.z / float(10)) * 0xFFFFFF);

	return draw_buffer;
}

vren::debug_renderer_draw_buffer vren_demo::create_debug_draw_buffer_with_demo_camera_clusters(vren::context const& context)
{
	const uint32_t k_camera_frustum_color = 0xffffff;
	const uint32_t k_camera_plane_color = 0xffffff;
	const uint32_t k_cluster_color = 0xffffff;
	const uint32_t k_cluster_count = 1024;

	const glm::uvec2 k_num_tiles(
		vren::divide_and_ceil(1920, 32),
		vren::divide_and_ceil(1080, 32)
	);

	vren::Camera camera{
		.m_near_plane = 0.1f,
		.m_far_plane = 10.0f,
	};

	glm::mat4 inv_view = glm::inverse(camera.get_view());
	glm::mat4 inv_proj = glm::inverse(camera.get_projection());

	auto to_world_space = [&](glm::vec3 const& ndc_position)
	{
		glm::vec4 tmp = inv_proj * glm::vec4(ndc_position, 1.0f);
		tmp /= tmp.w;
		return inv_view * tmp;
	};

	float tan_half_fov = glm::tan(camera.m_fov_y / 2.0f);
	
	vren::debug_renderer_draw_buffer draw_buffer(context);

	// Frustum border
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(1.0f, 1.0f, 1.0f)), .m_color = k_camera_frustum_color, });
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(-1.0f, 1.0f, 1.0f)), .m_color = k_camera_frustum_color, });
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(1.0f, -1.0f, 1.0f)), .m_color = k_camera_frustum_color, });
	draw_buffer.add_line({ .m_from = camera.m_position, .m_to = to_world_space(glm::vec3(-1.0f, -1.0f, 1.0f)), .m_color = k_camera_frustum_color, });

	// Near plane
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, 1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(1.0f, -1.0f, 0.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, -1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(-1.0f, -1.0f, 0.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, -1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(-1.0f, 1.0f, 0.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, 1.0f, 0.0f)), .m_to = to_world_space(glm::vec3(1.0f, 1.0f, 0.0f)), .m_color = k_camera_plane_color, });

	// Far plane
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, 1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(1.0f, -1.0f, 1.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(1.0f, -1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(-1.0f, -1.0f, 1.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, -1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(-1.0f, 1.0f, 1.0f)), .m_color = k_camera_plane_color, });
	draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(-1.0f, 1.0f, 1.0f)), .m_to = to_world_space(glm::vec3(1.0f, 1.0f, 1.0f)), .m_color = k_camera_plane_color, });

	/*
	for (uint32_t x = 0; x < 5; x++)
	{
		for (uint32_t y = 0; y < 5; y++)
		{
			glm::vec2
				tile_min{ 10 + x, 10 + y },
				tile_max = tile_min + 1.0f;

			tile_min /= num_tiles;
			tile_min.y = (1.0f - tile_min.y) * 2.0f - 1.0f;
			tile_min.x = tile_min.x * 2.0f - 1.0f;

			tile_max /= num_tiles;
			tile_max.y = (1.0f - tile_max.y) * 2.0f - 1.0f;
			tile_max.x = tile_max.x * 2.0f - 1.0f;

			draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(tile_min.x, tile_min.y, 0.0f)), .m_to = to_world_space(glm::vec3(tile_min.x, tile_max.y, 0.0f)), .m_color = camera_plane_color, });
			draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(tile_min.x, tile_max.y, 0.0f)), .m_to = to_world_space(glm::vec3(tile_max.x, tile_max.y, 0.0f)), .m_color = camera_plane_color, });
			draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(tile_max.x, tile_max.y, 0.0f)), .m_to = to_world_space(glm::vec3(tile_max.x, tile_min.y, 0.0f)), .m_color = camera_plane_color, });
			draw_buffer.add_line({ .m_from = to_world_space(glm::vec3(tile_max.x, tile_min.y, 0.0f)), .m_to = to_world_space(glm::vec3(tile_min.x, tile_min.y, 0.0f)), .m_color = camera_plane_color, });
		}
	}
	*/

	/*
	float tile_z = 0.2f;
	while (tile_z < 10.0f)
	{
		for (uint32_t x = 0; x < 3; x++)
		{
			for (uint32_t y = 0; y < 3; y++)
			{
				glm::vec2
					tile_min{ 10 + x, 10 + y },
					tile_max = tile_min + 1.0f;

				tile_min /= num_tiles;
				tile_min.y = (1.0f - tile_min.y) * 2.0f - 1.0f;
				tile_min.x = tile_min.x * 2.0f - 1.0f;

				glm::vec4 cluster_min{};
				cluster_min.x = tile_min.x * tan_half_fov * tile_z * camera.m_aspect_ratio;
				cluster_min.y = tile_min.y * tan_half_fov * tile_z;
				cluster_min.z = tile_z;
				cluster_min.w = 1.0f;

				tile_max /= num_tiles;
				tile_max.y = (1.0f - tile_max.y) * 2.0f - 1.0f;
				tile_max.x = tile_max.x * 2.0f - 1.0f;

				glm::vec4 cluster_max{};
				cluster_max.x = tile_max.x * tan_half_fov * tile_z * camera.m_aspect_ratio;
				cluster_max.y = tile_max.y * tan_half_fov * tile_z;
				cluster_max.z = tile_z;
				cluster_max.w = 1.0f;

				draw_buffer.add_line({ .m_from = inv_view * glm::vec4(cluster_min.x, cluster_min.y, tile_z, 0.0f), .m_to = inv_view * glm::vec4(cluster_min.x, cluster_max.y, tile_z, 0.0f), .m_color = camera_plane_color, });
				draw_buffer.add_line({ .m_from = inv_view * glm::vec4(cluster_min.x, cluster_max.y, tile_z, 0.0f), .m_to = inv_view * glm::vec4(cluster_max.x, cluster_max.y, tile_z, 0.0f), .m_color = camera_plane_color, });
				draw_buffer.add_line({ .m_from = inv_view * glm::vec4(cluster_max.x, cluster_max.y, tile_z, 0.0f), .m_to = inv_view * glm::vec4(cluster_max.x, cluster_min.y, tile_z, 0.0f), .m_color = camera_plane_color, });
				draw_buffer.add_line({ .m_from = inv_view * glm::vec4(cluster_max.x, cluster_min.y, tile_z, 0.0f), .m_to = inv_view * glm::vec4(cluster_min.x, cluster_min.y, tile_z, 0.0f), .m_color = camera_plane_color, });
			}
		}

		tile_z += 0.05f;
	}*/

	return draw_buffer;
}
