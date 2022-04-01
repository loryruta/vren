#include "renderer.hpp"

#include <array>

#include "context.hpp"
#include "material.hpp"
#include "utils/image.hpp"
#include "utils/misc.hpp"
#include "utils/vk_toolbox.hpp"

template<typename _t, size_t... _is> // TODO put in utilities?
auto create_array_impl(std::function<_t()> const& create_entry_fn, std::index_sequence<_is...> _)
{
    return std::array{ (_is, create_entry_fn())... };
}

template<typename _t, size_t _size>
auto create_array(std::function<_t()> const& create_entry_fn)
{
    return create_array_impl(create_entry_fn, std::make_index_sequence<_size>());
}

std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>
vren::renderer::_create_point_lights_buffers()
{
    return create_array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>(
        [&]() {
			return vren::vk_utils::alloc_host_visible_buffer(
				m_toolbox->m_context,
				// The usage should only be storage buffer, but from vren_demo we need to copy the buffer's content
				// through `vkCmdCopyBuffer`. For this reason we're also providing transfer usage. Copying is a common need,
				// does it make sense always to create buffers with copy support?
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VREN_POINT_LIGHTS_BUFFER_SIZE,
				true
			);
		}
    );
}

std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>
vren::renderer::_create_directional_lights_buffers()
{
    return create_array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>(
        [&]() { return vren::vk_utils::alloc_host_visible_buffer(m_toolbox->m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VREN_DIRECTIONAL_LIGHTS_BUFFER_SIZE, true); }
    );
}

std::array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>
vren::renderer::_create_spot_lights_buffers()
{
    return create_array<vren::vk_utils::buffer, VREN_MAX_FRAMES_IN_FLIGHT>(
        [&]() {
			return vren::vk_utils::alloc_host_visible_buffer(m_toolbox->m_context, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VREN_SPOT_LIGHTS_BUFFER_SIZE, true);
		}
    );
}

vren::renderer::renderer(std::shared_ptr<vren::vk_utils::toolbox> const& toolbox) :
	m_toolbox(toolbox),

	m_render_pass(std::make_shared<vren::vk_render_pass>(_create_render_pass())),

	m_point_lights_buffers(_create_point_lights_buffers()),
	m_directional_lights_buffers(_create_directional_lights_buffers()),
	m_spot_lights_buffers(_create_spot_lights_buffers())
{}

vren::renderer::~renderer()
{}

void vren::renderer::_init()
{
	m_simple_draw_pass = std::make_unique<vren::simple_draw_pass>(m_toolbox, *this);
}

vren::vk_render_pass vren::renderer::_create_render_pass()
{
	auto& ctx = m_toolbox->m_context;

	VkAttachmentDescription color_attachment{};
	color_attachment.format = VREN_COLOR_BUFFER_OUTPUT_FORMAT;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = VREN_DEPTH_BUFFER_OUTPUT_FORMAT;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription attachments[]{
		color_attachment,
		depth_attachment
	};

	// ---------------------------------------------------------------- Subpasses

	std::vector<VkSubpassDescription> subpasses;

    // simple_draw
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;
    subpasses.push_back(subpass);

	// ---------------------------------------------------------------- Dependencies

	std::vector<VkSubpassDependency> dependencies;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies.push_back(dependency);

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = std::size(attachments);
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = subpasses.size();
	render_pass_info.pSubpasses = subpasses.data();
	render_pass_info.dependencyCount = dependencies.size();
	render_pass_info.pDependencies = dependencies.data();

	VkRenderPass render_pass;
	vren::vk_utils::check(vkCreateRenderPass(ctx->m_device, &render_pass_info, nullptr, &render_pass));
	return vren::vk_render_pass(ctx, render_pass);
}

void vren::renderer::_upload_lights_array(int frame_idx, vren::light_array const& lights_arr)
{
	auto& ctx = m_toolbox->m_context;

    /* Upload point lights */
    uint32_t point_lights_count = lights_arr.m_point_lights.size();
    if (point_lights_count > k_max_point_lights_count) {
        throw std::runtime_error("Too many point lights");
    }
    vren::vk_utils::update_host_visible_buffer(*ctx, m_point_lights_buffers[frame_idx], &point_lights_count, sizeof(uint32_t), 0);
    vren::vk_utils::update_host_visible_buffer(*ctx, m_point_lights_buffers[frame_idx], lights_arr.m_point_lights.data(), point_lights_count * sizeof(vren::point_light), sizeof(uint32_t)  + (sizeof(float) * 3));

    /* Upload directional lights */
    uint32_t dir_lights_count = lights_arr.m_directional_lights.size();
    if (dir_lights_count > k_max_directional_lights_count) {
        throw std::runtime_error("Too many directional lights");
    }
    vren::vk_utils::update_host_visible_buffer(*ctx, m_directional_lights_buffers[frame_idx], &dir_lights_count, sizeof(uint32_t), 0);
    vren::vk_utils::update_host_visible_buffer(*ctx, m_directional_lights_buffers[frame_idx], lights_arr.m_directional_lights.data(), dir_lights_count * sizeof(vren::directional_light), sizeof(uint32_t)  + (sizeof(float) * 3));

    /* Upload spotlights */
    uint32_t spot_lights_count = lights_arr.m_spot_lights.size();
    if (lights_arr.m_spot_lights.size() > k_max_spot_lights_count) {
        throw std::runtime_error("Too many spot lights");
    }
    vren::vk_utils::update_host_visible_buffer(*ctx, m_spot_lights_buffers[frame_idx], &spot_lights_count, sizeof(uint32_t), 0);
    vren::vk_utils::update_host_visible_buffer(*ctx, m_spot_lights_buffers[frame_idx], lights_arr.m_spot_lights.data(), spot_lights_count * sizeof(vren::spot_light), sizeof(uint32_t)  + (sizeof(float) * 3));
}

void vren::renderer::write_light_array_descriptor_set(uint32_t frame_idx, VkDescriptorSet desc_set)
{
	auto& ctx = m_toolbox->m_context;

	VkDescriptorBufferInfo buf_info[]{
		{ /* Point lights */
			.buffer = m_point_lights_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range  = VK_WHOLE_SIZE
		},
		{ /* Dir. lights */
			.buffer = m_directional_lights_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range  = VK_WHOLE_SIZE
		},
		{ /* Spot lights */
			.buffer = m_spot_lights_buffers.at(frame_idx).m_buffer.m_handle,
			.offset = 0,
			.range  = VK_WHOLE_SIZE
		}
	};

	VkWriteDescriptorSet write_desc_set{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = desc_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = std::size(buf_info),
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = buf_info,
		.pTexelBufferView = nullptr
	};
	vkUpdateDescriptorSets(ctx->m_device, 1, &write_desc_set, 0, nullptr);
}

void vren::renderer::render(
	int frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	vren::render_target const& target,
	vren::render_list const& render_list,
	vren::light_array const& lights_arr,
	vren::camera const& camera
)
{
	/* Render pass begin */
	VkClearValue clear_values[] = {
		{ .color = m_clear_color },
		{ .depthStencil = {
			.depth = 1.0f,
			.stencil = 0
		}}
	};
	VkRenderPassBeginInfo render_pass_begin_info{};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass = m_render_pass->m_handle;
	render_pass_begin_info.framebuffer = target.m_framebuffer;
	render_pass_begin_info.renderArea = target.m_render_area;
	render_pass_begin_info.clearValueCount = std::size(clear_values);
	render_pass_begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd_buf, 0, 1, &target.m_viewport);
	vkCmdSetScissor(cmd_buf, 0, 1, &target.m_render_area);

	/* Write light buffers */
	_upload_lights_array(frame_idx, lights_arr);

	/* Subpass recording */
	m_simple_draw_pass->record_commands(frame_idx, cmd_buf, res_container, render_list, lights_arr, camera);

	/* Render pass end */
	vkCmdEndRenderPass(cmd_buf);
}

std::shared_ptr<vren::renderer>
vren::renderer::create(std::shared_ptr<vren::vk_utils::toolbox> const& toolbox)
{
	auto renderer = std::shared_ptr<vren::renderer>(new vren::renderer(toolbox));
	renderer->_init();
	return renderer;
}
