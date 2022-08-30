#include "clustered_shading_debug.hpp"

#include "toolbox.hpp"

vren_demo::visualize_clusters::visualize_clusters(
    vren::context const& context
) :
    m_context(&context),
	m_pipeline([&]()
	{
		vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/visualize_clusters.comp.spv");
		vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
		return vren::create_compute_pipeline(*m_context, shader);
	}())
{
}

vren::render_graph_t vren_demo::visualize_clusters::operator()(
	vren::render_graph_allocator& render_graph_allocator,
	glm::uvec2 const& screen,
	uint32_t mode,
	vren::vk_utils::combined_image_view const& cluster_reference_buffer,
	vren::vk_utils::buffer const& cluster_key_buffer,
	vren::vk_utils::buffer const& assigned_light_buffer,
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
		&cluster_reference_buffer,
		&cluster_key_buffer,
		&assigned_light_buffer,
		&output
	](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkBufferMemoryBarrier buffer_memory_barriers[]{
			VkBufferMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.buffer = cluster_key_buffer.m_buffer.m_handle,
				.offset = 0,
				.size = VK_WHOLE_SIZE
			},
			VkBufferMemoryBarrier{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.buffer = assigned_light_buffer.m_buffer.m_handle,
				.offset = 0,
				.size = VK_WHOLE_SIZE
			},
		};

		VkImageMemoryBarrier image_memory_barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
			.newLayout = VK_IMAGE_LAYOUT_GENERAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = cluster_reference_buffer.get_image(),
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1,
			}
		};
		vkCmdPipelineBarrier(
			command_buffer,
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			NULL,
			0, nullptr,
			std::size(buffer_memory_barriers), buffer_memory_barriers,
			1, &image_memory_barrier
		);

		m_pipeline.bind(command_buffer);

		struct
		{
			uint32_t m_mode;
		} push_constants;
		push_constants = {
			.m_mode = mode,
		};
		m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants));

		auto descriptor_set_0 = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
		);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 0, cluster_reference_buffer.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 1, cluster_key_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 2, assigned_light_buffer.m_buffer.m_handle, VK_WHOLE_SIZE, 0);
		vren::vk_utils::write_storage_image_descriptor(*m_context, descriptor_set_0->m_handle.m_descriptor_set, 3, output.m_image_view.m_handle, VK_IMAGE_LAYOUT_GENERAL);

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

	vren::camera camera{
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

	// Clusters
	for (uint32_t i = 0; i < k_cluster_count; i++)
	{
		float sample_z = (float(std::rand()) / float(RAND_MAX)) * (camera.m_far_plane - camera.m_near_plane) + camera.m_near_plane;

		glm::uvec3 cluster_ijk{};
		//cluster_ijk.x = 20;
		//cluster_ijk.y = 6;
		//cluster_ijk.z = 23;//glm::uint(glm::floor(glm::log(sample_z / camera.m_near_plane) / glm::log(1.0f + (2.0f * tan_half_fov) / float(num_tiles.y))));
		cluster_ijk.x = std::rand() % k_num_tiles.x;
		cluster_ijk.y = std::rand() % k_num_tiles.y;
		cluster_ijk.z = 128;//std::rand() % 1024;

		glm::vec2 tile_min = glm::vec2(cluster_ijk.x, cluster_ijk.y);
		tile_min /= k_num_tiles;
		tile_min.y = (1.0 - tile_min.y) * 2.0 - 1.0;
		tile_min.x = tile_min.x * 2.0 - 1.0;

		glm::vec2 tile_max = glm::vec2(cluster_ijk.x, cluster_ijk.y) + glm::vec2(1.0f);
		tile_max /= k_num_tiles;
		tile_max.y = (1.0 - tile_max.y) * 2.0 - 1.0;
		tile_max.x = tile_max.x * 2.0 - 1.0;

		glm::vec2 tmp = tile_min;
		tile_min = glm::min(tile_min, tile_max);
		tile_max = glm::max(tile_max, tmp);

		float near_k = camera.m_near_plane * glm::pow(1.0f + ((2.0f * tan_half_fov) / float(k_num_tiles.y)), float(cluster_ijk.z));
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

		draw_buffer.add_cube({ .m_min = cluster_min, .m_max = cluster_max, .m_color = k_cluster_color, });
	}

	return draw_buffer;
}
