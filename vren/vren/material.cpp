#include "material.hpp"

#include "base/base.hpp"
#include "texture_manager.hpp"
#include "vk_helpers/buffer.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/debug_utils.hpp"
#include "vk_helpers/barrier.hpp"

vren::material_manager::material_manager(vren::context const& context) :
	m_context(&context),
	m_descriptor_set_layout(create_descriptor_set_layout()),
	m_descriptor_pool(create_descriptor_pool()),
	m_staging_buffer(create_staging_buffer()),
	m_materials(static_cast<vren::material*>(m_staging_buffer.m_allocation_info.pMappedData)),
	m_buffers(create_buffers()),
	m_descriptor_sets(allocate_descriptor_sets())
{
	for (uint32_t i = 0; i < m_descriptor_sets.size(); i++) {
		write_descriptor_set(i);
	}
}

vren::vk_utils::buffer vren::material_manager::create_staging_buffer()
{
	return vren::vk_utils::alloc_host_visible_buffer(*m_context, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, k_max_material_buffer_size, true);
}

std::array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::material_manager::create_buffers()
{
	return vren::create_array<vren::vk_utils::buffer, VREN_MAX_FRAME_IN_FLIGHT_COUNT>([&](uint32_t idx)
	{
		auto buffer = vren::vk_utils::alloc_device_only_buffer(*m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, k_max_material_buffer_size);

		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_BUFFER, (uint64_t) buffer.m_buffer.m_handle, fmt::format("Material buffer {}", idx).c_str());

		return std::move(buffer);
	});
}

vren::vk_descriptor_set_layout vren::material_manager::create_descriptor_set_layout()
{
	VkDescriptorSetLayoutBinding bindings[]{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_ALL,
			.pImmutableSamplers = nullptr
		},
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

vren::vk_descriptor_pool vren::material_manager::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[]{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VREN_MAX_FRAME_IN_FLIGHT_COUNT },
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

std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> vren::material_manager::allocate_descriptor_sets()
{
	std::array<VkDescriptorSet, VREN_MAX_FRAME_IN_FLIGHT_COUNT> descriptor_sets;

	std::array<VkDescriptorSetLayout, VREN_MAX_FRAME_IN_FLIGHT_COUNT> descriptor_set_layouts;
	std::fill(descriptor_set_layouts.begin(), descriptor_set_layouts.end(), m_descriptor_set_layout.m_handle);

	VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = m_descriptor_pool.m_handle,
		.descriptorSetCount = std::size(descriptor_sets),
		.pSetLayouts = descriptor_set_layouts.data()
	};
	VREN_CHECK(vkAllocateDescriptorSets(m_context->m_device, &descriptor_set_alloc_info, descriptor_sets.data()), m_context);

	for (auto const& descriptor_set : descriptor_sets) {
		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptor_set, "material_manager");
	}

	return descriptor_sets;
}

void vren::material_manager::write_descriptor_set(uint32_t frame_idx)
{
	VkDescriptorBufferInfo buffer_info{
		.buffer = m_buffers.at(frame_idx).m_buffer.m_handle,
		.offset = 0,
		.range = m_material_count > 0 ? m_material_count * sizeof(vren::material) : 1
	};
	VkWriteDescriptorSet descriptor_set_write{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = m_descriptor_sets.at(frame_idx),
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &buffer_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}

void vren::material_manager::lazy_initialize()
{
	// Default material
	m_materials[m_material_count++] = {
		.m_base_color_texture_idx = vren::texture_manager::k_white_texture,
		.m_metallic_roughness_texture_idx = vren::texture_manager::k_white_texture,
	};
}

void vren::material_manager::request_buffer_sync()
{
	m_buffers_dirty_flags = UINT32_MAX;
}

vren::render_graph_t vren::material_manager::sync_buffer(vren::render_graph_allocator& allocator, uint32_t frame_idx)
{
	vren::render_graph_t result;

	if ((m_buffers_dirty_flags >> frame_idx) & 1)
	{
		auto node = allocator.allocate();
		node->set_src_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
		node->set_dst_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
		node->set_name("Material buffer uploading");
		node->add_buffer({
			.m_buffer = m_staging_buffer.m_buffer.m_handle
		}, VK_ACCESS_TRANSFER_READ_BIT);
		node->add_buffer({
			.m_buffer = m_buffers.at(frame_idx).m_buffer.m_handle,
		}, VK_ACCESS_TRANSFER_WRITE_BIT);
		node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
		{
			if (m_material_count > 0)
			{
				VkBufferCopy buffer_copy{ .srcOffset = 0, .dstOffset = 0, .size = m_material_count * sizeof(vren::material) };
				vkCmdCopyBuffer(command_buffer, m_staging_buffer.m_buffer.m_handle, m_buffers.at(frame_idx).m_buffer.m_handle, 1, &buffer_copy);
			}

			write_descriptor_set(frame_idx); // The descriptor set must be re-written since the material count changed
		});
		result = vren::render_graph_gather(node);

		m_buffers_dirty_flags &= ~(1 << frame_idx);
	}

	return result;
}

VkDescriptorSet vren::material_manager::get_descriptor_set(uint32_t frame_idx) const
{
	return m_descriptor_sets.at(frame_idx);
}

VkBuffer vren::material_manager::get_buffer(uint32_t frame_idx) const
{
	return m_buffers.at(frame_idx).m_buffer.m_handle;
}
