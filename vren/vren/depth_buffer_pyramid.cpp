#include "depth_buffer_pyramid.hpp"

#include <glm/gtc/integer.hpp>

#include "toolbox.hpp"
#include "base/base.hpp"
#include "vk_helpers/misc.hpp"
#include "vk_helpers/barrier.hpp"
#include "vk_helpers/debug_utils.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Depth buffer pyramid
// --------------------------------------------------------------------------------------------------------------------------------

vren::depth_buffer_pyramid::depth_buffer_pyramid(vren::context const& context, uint32_t width, uint32_t height) :
	m_context(&context),
	m_base_width(width),
	m_base_height(height),
	m_level_count(glm::log2(glm::max(width, height))),
	m_image(create_image()),
	m_image_views(create_image_views())
{
	assert(width % 32 == 0);
	assert(height % 32 == 0);
}

vren::vk_utils::image vren::depth_buffer_pyramid::create_image()
{
	VkImageCreateInfo image_info{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R32_SFLOAT,
		.extent = {
			.width = m_base_width,
			.height = m_base_height,
			.depth = 1,
		},
		.mipLevels = m_level_count,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Debug

	VmaAllocationCreateInfo allocation_info{
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	VkImage image;
	VmaAllocation allocation;
	VREN_CHECK(vmaCreateImage(m_context->m_vma_allocator, &image_info, &allocation_info, &image, &allocation, nullptr), m_context);

	vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_IMAGE, (uint64_t) image, "depth_buffer_pyramid");

	return {
		.m_image = vren::vk_image(*m_context, image),
		.m_allocation = vren::vma_allocation(*m_context, allocation)
	};
}

std::vector<vren::vk_image_view> vren::depth_buffer_pyramid::create_image_views()
{
	std::vector<vren::vk_image_view> image_views;

	for (uint32_t i = 0; i < m_level_count; i++)
	{
		VkImageViewCreateInfo image_view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = NULL,
			.image = m_image.m_image.m_handle,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R32_SFLOAT,
			.components = {},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = i,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		VkImageView image_view;
		VREN_CHECK(vkCreateImageView(m_context->m_device, &image_view_info, nullptr, &image_view), m_context);

		vren::vk_utils::set_object_name(*m_context, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) image_view, fmt::format("depth_buffer_pyramid level={}", i).c_str());

		image_views.push_back(vren::vk_image_view(*m_context, image_view));
	}

	return image_views;
}

// --------------------------------------------------------------------------------------------------------------------------------
// Depth buffer reductor
// --------------------------------------------------------------------------------------------------------------------------------

vren::depth_buffer_reductor::depth_buffer_reductor(vren::context const& context) :
	m_context(&context),
	m_copy_pipeline(create_copy_pipeline()),
	m_reduce_pipeline(create_reduce_pipeline()),
	m_depth_buffer_sampler(create_depth_buffer_sampler())
{}

vren::vk_utils::pipeline vren::depth_buffer_reductor::create_copy_pipeline()
{
	return vren::vk_utils::create_compute_pipeline(
		*m_context,
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/depth_buffer_copy.comp.spv")
	);
}

vren::vk_utils::pipeline vren::depth_buffer_reductor::create_reduce_pipeline()
{
	return vren::vk_utils::create_compute_pipeline(
		*m_context,
		vren::vk_utils::load_shader_from_file(*m_context, ".vren/resources/shaders/depth_buffer_reduce.comp.spv")
	);
}

vren::vk_sampler vren::depth_buffer_reductor::create_depth_buffer_sampler()
{
	VkSamplerCreateInfo sampler_info{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		.unnormalizedCoordinates = true
	};
	VkSampler sampler;
	VREN_CHECK(vkCreateSampler(m_context->m_device, &sampler_info, nullptr, &sampler), m_context);
	return vren::vk_sampler(*m_context, sampler);
}

void write_copy_pipeline_descriptor_set(
	vren::context const& context,
	VkDescriptorSet descriptor_set,
	VkImageView depth_buffer,
	VkSampler depth_buffer_sampler,
	VkImageView depth_buffer_pyramid_base
)
{
	VkDescriptorImageInfo image_info;
	VkWriteDescriptorSet write_descriptor_set;

	// Depth buffer
	image_info = {
		.sampler = depth_buffer_sampler,
		.imageView = depth_buffer,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
	write_descriptor_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptor_set,
		.dstBinding = 0, // Binding 0
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &image_info,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(context.m_device, 1, &write_descriptor_set, 0, nullptr);

	// Depth buffer pyramid base
	image_info = {
		.sampler = VK_NULL_HANDLE,
		.imageView = depth_buffer_pyramid_base,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};
	write_descriptor_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptor_set,
		.dstBinding = 1, // Binding 1
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &image_info,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(context.m_device, 1, &write_descriptor_set, 0, nullptr);
}

void write_reduce_pipeline_descriptor_set(
	vren::context const& context,
	VkDescriptorSet descriptor_set,
	VkImageView from_image,
	VkImageView to_image
)
{
	VkDescriptorImageInfo image_info[]{
		{ // Binding 0
			.sampler = VK_NULL_HANDLE,
			.imageView = from_image,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
		},
		{ // Binding 1
			.sampler = VK_NULL_HANDLE,
			.imageView = to_image,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
		}
	};
	VkWriteDescriptorSet write_descriptor_set{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = image_info,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(context.m_device, 1, &write_descriptor_set, 0, nullptr);
}

vren::render_graph_node* vren::depth_buffer_reductor::copy_depth_buffer_to_depth_buffer_pyramid_base(vren::vk_utils::depth_buffer_t const& depth_buffer, vren::depth_buffer_pyramid const& depth_buffer_pyramid) const
{
	auto node = new vren::render_graph_node();
	node->set_name("depth_buffer_reductor | copy_depth_buffer_to_depth_buffer_pyramid_base");
	node->set_in_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->set_out_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->add_image(
		{ .m_name = "depth_buffer", .m_image = depth_buffer.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT },
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_ACCESS_SHADER_READ_BIT
	);
	node->add_image(
		{ .m_name = "depth_buffer_pyramid level=0", .m_image = depth_buffer_pyramid.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = 0 },
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_SHADER_WRITE_BIT
	);
	node->set_callback([&](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		m_copy_pipeline.bind(command_buffer);

		auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_copy_pipeline.m_descriptor_set_layouts.at(0))
		);
		resource_container.add_resource(descriptor_set);
		write_copy_pipeline_descriptor_set(*m_context, descriptor_set->m_handle.m_descriptor_set, depth_buffer.m_image_view.m_handle, m_depth_buffer_sampler.m_handle, depth_buffer_pyramid.m_image_views.at(0).m_handle);
		m_copy_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

		m_copy_pipeline.dispatch(command_buffer, depth_buffer_pyramid.m_base_width >> 5, depth_buffer_pyramid.m_base_height >> 5, 1);
	});
	return node;
}

vren::render_graph_node* vren::depth_buffer_reductor::reduce_step(vren::depth_buffer_pyramid const& depth_buffer_pyramid, uint32_t current_level) const
{
	auto node = new vren::render_graph_node();
	node->set_name(fmt::format("depth_buffer_reductor | reduce_step {} -> {}", current_level, current_level + 1));
	node->set_in_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->set_out_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	node->add_image(
		{ .m_name = fmt::format("depth_buffer_pyramid level={}", current_level), .m_image = depth_buffer_pyramid.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = current_level },
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_SHADER_READ_BIT
	);
	node->add_image(
		{ .m_name = fmt::format("depth_buffer_pyramid level={}", current_level + 1), .m_image = depth_buffer_pyramid.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = current_level + 1 },
		VK_IMAGE_LAYOUT_GENERAL,
		VK_ACCESS_SHADER_WRITE_BIT
	);
	node->set_callback([=, &depth_buffer_pyramid](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		m_reduce_pipeline.bind(command_buffer);

		auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
			m_context->m_toolbox->m_descriptor_pool.acquire(m_reduce_pipeline.m_descriptor_set_layouts.at(0))
		);
		resource_container.add_resource(descriptor_set);
		write_reduce_pipeline_descriptor_set(
			*m_context,
			descriptor_set->m_handle.m_descriptor_set,
			depth_buffer_pyramid.m_image_views.at(current_level).m_handle,
			depth_buffer_pyramid.m_image_views.at(current_level + 1).m_handle
		);
		m_reduce_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

		m_reduce_pipeline.dispatch(command_buffer, depth_buffer_pyramid.m_base_width >> (5 + current_level + 1), depth_buffer_pyramid.m_base_height >> (5 + current_level + 1), 1);
	});
	return node;
}

vren::render_graph_node* vren::depth_buffer_reductor::reduce(vren::depth_buffer_pyramid const& depth_buffer_pyramid) const
{
	vren::render_graph_node* head = nullptr;
	if (depth_buffer_pyramid.m_level_count > 1)
	{
		head = reduce_step(depth_buffer_pyramid, 0);

		vren::render_graph_node* current = head;
		for (uint32_t i = 1; i < depth_buffer_pyramid.m_level_count - 1; i++)
		{
			auto following = reduce_step(depth_buffer_pyramid, i);
			current->add_following(following);
			current = following;
		}
	}
	return head;
}

vren::render_graph_node* vren::depth_buffer_reductor::copy_and_reduce(vren::vk_utils::depth_buffer_t const& depth_buffer, vren::depth_buffer_pyramid const& depth_buffer_pyramid) const
{
	auto head = copy_depth_buffer_to_depth_buffer_pyramid_base(depth_buffer, depth_buffer_pyramid);
	head->add_following(
		reduce(depth_buffer_pyramid)
	);
	return head;
}

// --------------------------------------------------------------------------------------------------------------------------------

vren::render_graph_node* vren::blit_depth_buffer_pyramid_level_to_color_buffer(vren::depth_buffer_pyramid const& depth_buffer_pyramid, uint32_t level, vren::vk_utils::color_buffer_t const& color_buffer, uint32_t width, uint32_t height)
{
	auto node = new vren::render_graph_node();
	node->set_name(fmt::format("blit_depth_buffer_pyramid_level_to_color_buffer level={}", level));
	node->set_in_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->set_out_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);
	node->add_image(
		{ .m_name = fmt::format("depth_buffer_pyramid level={}", level), .m_image = depth_buffer_pyramid.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT, .m_mip_level = level },
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_ACCESS_TRANSFER_READ_BIT
	);
	node->add_image(
		{ .m_name = "color_buffer", .m_image = color_buffer.get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT },
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_ACCESS_TRANSFER_WRITE_BIT
	);
	node->set_callback([=, &depth_buffer_pyramid, &color_buffer](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		VkImageBlit image_blit{
			.srcSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = level, .baseArrayLayer = 0, .layerCount = 1 },
			.srcOffsets = {
				{ 0, 0, 0 },
				{ (int32_t) depth_buffer_pyramid.get_image_width(level), (int32_t) depth_buffer_pyramid.get_image_height(level), 1 },
			},
			.dstSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1, },
			.dstOffsets = {
				{ 0, 0, 0 },
				{ (int32_t) width, (int32_t) height, 1 }
			},
		};
		vkCmdBlitImage(command_buffer, depth_buffer_pyramid.get_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, color_buffer.get_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit, VK_FILTER_NEAREST);
	});
	return node;
}

