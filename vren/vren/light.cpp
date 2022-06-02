#include "light.hpp"

#include "base/base.hpp"
#include "context.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"

vren::light_array::light_array(vren::context const& context) :
	m_context(&context),

	// Staging buffers
	m_point_light_staging_buffer      (create_staging_buffer(k_point_light_buffer_size, "Point light staging buffer")),
	m_directional_light_staging_buffer(create_staging_buffer(k_point_light_buffer_size, "Directional light staging buffer")),
	m_spot_light_staging_buffer       (create_staging_buffer(k_point_light_buffer_size, "Spot light staging buffer")),

	// GPU buffers
	m_point_light_buffers      (create_buffers(k_point_light_buffer_size, "Point light buffer")),
	m_directional_light_buffers(create_buffers(k_directional_light_buffer_size, "Directional light buffer")),
	m_spot_light_buffers       (create_buffers(k_spot_light_buffer_size, "Spot light buffer")),

	// Persistent maps
	m_point_lights      (static_cast<vren::point_light*>(m_point_light_staging_buffer.m_allocation_info.pMappedData)),
	m_directional_lights(static_cast<vren::directional_light*>(m_point_light_staging_buffer.m_allocation_info.pMappedData)),
	m_spot_lights       (static_cast<vren::spot_light*>(m_point_light_staging_buffer.m_allocation_info.pMappedData)),

	m_descriptor_set_layout(create_descriptor_set_layout()),
	m_descriptor_pool(create_descriptor_pool()),
	m_descriptor_sets(allocate_descriptor_sets())
{
	for (uint32_t i = 0; i < m_descriptor_sets.size(); i++)
	{
		write_descriptor_set(i);
	}
}

vren::vk_utils::buffer vren::light_array::create_staging_buffer(size_t buffer_size, std::string const& buffer_name)
{
	auto buffer = vren::vk_utils::alloc_host_visible_buffer(*m_context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, buffer_size, true);

	vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, buffer_name.c_str());

	return std::move(buffer);
}

std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::light_array::create_buffers(size_t buffer_size, std::string const& base_buffer_name)
{
	return create_array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&](uint32_t idx)
	{
		auto buffer = vren::vk_utils::alloc_device_only_buffer(*m_context, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer_size);

		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, (base_buffer_name + fmt::format(" {}", idx)).c_str());

		return std::move(buffer);
	});
}

vren::vk_descriptor_set_layout vren::light_array::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding bindings[]{
		{ // Point lights
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		{ // Directional lights
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
		{ // Spot lights
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		}
	};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.bindingCount = std::size(bindings),
		.pBindings = bindings
	};
	VkDescriptorSetLayout descriptor_set_layout;
	VREN_CHECK(vkCreateDescriptorSetLayout(m_context->m_device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout), m_context);
	return vren::vk_descriptor_set_layout(*m_context, descriptor_set_layout);
}

vren::vk_descriptor_pool vren::light_array::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[]{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
	};

	VkDescriptorPoolCreateInfo descriptor_pool_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.maxSets = VREN_MAX_FRAME_IN_FLIGHT_COUNT,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	VkDescriptorPool descriptor_pool;
	VREN_CHECK(vkCreateDescriptorPool(m_context->m_device, &descriptor_pool_info, nullptr, &descriptor_pool), m_context);
	return vren::vk_descriptor_pool(*m_context, descriptor_pool);
}

std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::light_array::allocate_descriptor_sets()
{
	std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> descriptor_sets;

	auto descriptor_set_layouts = vren::create_array<VkDescriptorSetLayout, VREN_MAX_FRAME_IN_FLIGHT_COUNT>(m_descriptor_set_layout.m_handle);
	VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = m_descriptor_pool.m_handle,
		.descriptorSetCount = std::size(descriptor_sets),
		.pSetLayouts = descriptor_set_layouts.data()
	};
	VREN_CHECK(vkAllocateDescriptorSets(m_context->m_device, &descriptor_set_alloc_info, descriptor_sets.data()), m_context);

	for (auto const& descriptor_set : descriptor_sets) {
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_set, "light_array");
	}

	return descriptor_sets;
}

void vren::light_array::write_descriptor_set(uint32_t frame_idx)
{
	VkDescriptorBufferInfo buffer_info[]{
		{ // Point lights
			.buffer = m_point_light_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range = m_point_light_count > 0 ? m_point_light_count * sizeof(vren::point_light) : 1
		},
		{ // Directional lights
			.buffer = m_directional_light_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range = m_directional_light_count > 0 ? m_directional_light_count * sizeof(vren::directional_light) : 1
		},
		{ // Spot lights
			.buffer = m_spot_light_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range = m_spot_light_count > 0 ? m_spot_light_count * sizeof(vren::spot_light) : 1
		}
	};
	VkWriteDescriptorSet descriptor_set_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptor_sets.at(frame_idx),
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = std::size(buffer_info),
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::light_array::request_point_light_buffer_sync()
{
	m_point_light_buffers_dirty_flags = UINT32_MAX;
}

void vren::light_array::request_directional_light_buffer_sync()
{
	m_directional_light_buffers_dirty_flags = UINT32_MAX;
}

void vren::light_array::request_spot_light_buffer_sync()
{
	m_point_light_buffers_dirty_flags = UINT32_MAX;
}

vren::render_graph_t vren::light_array::create_sync_light_buffer_node(
	vren::render_graph_allocator& allocator,
	char const* node_name,
	vren::vk_utils::buffer const& light_staging_buffer,
	vren::vk_utils::buffer const& light_gpu_buffer,
	size_t single_light_size,
	uint32_t light_count
)
{
	assert(single_light_size > 0);
	assert(light_count > 0);

	auto node = allocator.allocate();
	node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_name(node_name);
	node->add_buffer({ .m_buffer = light_staging_buffer.m_buffer.m_handle, }, VK_ACCESS_TRANSFER_READ_BIT);
	node->add_buffer({ .m_buffer = light_gpu_buffer.m_buffer.m_handle, }, VK_ACCESS_TRANSFER_WRITE_BIT);
	node->set_callback([=, &light_staging_buffer, &light_gpu_buffer](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkBufferCopy buffer_copy{ .srcOffset = 0, .dstOffset = 0, .size = light_count * single_light_size };
		vkCmdCopyBuffer(command_buffer, light_staging_buffer.m_buffer.m_handle, light_gpu_buffer.m_buffer.m_handle, 1, &buffer_copy);
	});

	return vren::render_graph_gather(node);
}

vren::render_graph_t vren::light_array::sync_buffers(
	vren::render_graph_allocator& allocator,
	uint32_t frame_idx
)
{
	vren::render_graph_t head, tail;

	bool something_synced = false;

	// Point lights
	if ((m_point_light_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_point_light_count > 0)
		{
			auto node = create_sync_light_buffer_node(
				allocator,
				"Point lights uploading",
				m_point_light_staging_buffer,
				m_point_light_buffers.at(frame_idx),
				sizeof(vren::point_light),
				m_point_light_count
			);

			tail = vren::render_graph_concat(allocator, tail, node);
			if (head.empty()) head = tail;
		}

		something_synced = true;

		m_point_light_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	// Dir. lights sync
	if ((m_directional_light_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_directional_light_count > 0)
		{
			auto node = create_sync_light_buffer_node(
				allocator,
				"Dir. lights uploading",
				m_directional_light_staging_buffer,
				m_directional_light_buffers.at(frame_idx),
				sizeof(vren::directional_light),
				m_directional_light_count
			);

			tail = vren::render_graph_concat(allocator, tail, node);
			if (head.empty()) head = tail;
		}

		something_synced = true;

		m_directional_light_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	// Spot lights sync
	if ((m_spot_light_buffers_dirty_flags >> frame_idx) & 1)
	{
		if (m_spot_light_count > 0)
		{
			auto node = create_sync_light_buffer_node(
				allocator,
				"Spot lights uploading",
				m_spot_light_staging_buffer,
				m_spot_light_buffers.at(frame_idx),
				sizeof(vren::spot_light),
				m_spot_light_count
			);

			tail = vren::render_graph_concat(allocator, tail, node);
			if (head.empty()) head = tail;
		}

		something_synced = true;

		m_spot_light_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	// Re-write descriptor set if something has changed
	if (something_synced)
	{
		auto node = allocator.allocate();
		node->set_callback([this](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container) {
			write_descriptor_set(frame_idx);
		});

		tail = vren::render_graph_concat(allocator, tail, vren::render_graph_gather(node));
		if (head.empty()) head = tail;
	}

	(void) tail;

	//
	return head;
}

VkDescriptorSet vren::light_array::get_descriptor_set(uint32_t frame_idx) const
{
	return m_descriptor_sets.at(frame_idx);
}
