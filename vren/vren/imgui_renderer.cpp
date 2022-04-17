#include "imgui_renderer.hpp"

#include <volk.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>

#include "context.hpp"
#include "vk_helpers/misc.hpp"

// References:
// - https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
// - https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.h
// - https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp

// --------------------------------------------------------------------------------------------------------------------------------
// ImGui renderer
// --------------------------------------------------------------------------------------------------------------------------------

vren::vk_render_pass vren::imgui_renderer::create_render_pass()
{
	VkAttachmentDescription color_attachment{};
	color_attachment.format = VREN_COLOR_BUFFER_OUTPUT_FORMAT;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
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
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Subpass
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

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::vector<VkAttachmentDescription> attachments{
		color_attachment,
		depth_attachment
	};

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = attachments.size();
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	VkRenderPass render_pass;
	VREN_CHECK(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &render_pass), m_context);
	return vren::vk_render_pass(*m_context, render_pass);
}

vren::imgui_renderer::imgui_renderer(vren::context const& ctx, GLFWwindow* window) :
	m_context(&ctx),
	m_window(window),
	m_render_pass(create_render_pass())
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = nullptr;

	ImPlot::CreateContext();

	ImGui_ImplVulkan_LoadFunctions([](char const* func_name, void* user_data) {
		return (PFN_vkVoidFunction) vkGetInstanceProcAddr((VkInstance) user_data, func_name);
	}, ctx.m_instance);

	ImGui_ImplGlfw_InitForVulkan(window, true);

	/* Create ImGui descriptor pool */
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1024;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	VREN_CHECK(vkCreateDescriptorPool(ctx.m_device, &pool_info, nullptr, &m_descriptor_pool), m_context);

	/* Create ImGui render pass */
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = ctx.m_instance;
	init_info.PhysicalDevice = ctx.m_physical_device;
	init_info.Device = ctx.m_device;
	init_info.QueueFamily = ctx.m_queue_families.m_graphics_idx;
	init_info.Queue = ctx.m_graphics_queue;
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = m_descriptor_pool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 3; // Why do you need to know about image count here, dear imgui vulkan backend...?
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vren::vk_utils::check;
	ImGui_ImplVulkan_Init(&init_info, m_render_pass.m_handle);

	vren::vk_utils::immediate_graphics_queue_submit(*m_context, [&](VkCommandBuffer cmd_buf, vren::resource_container& res_container) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd_buf);
	});

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

vren::imgui_renderer::~imgui_renderer()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(m_context->m_device, m_descriptor_pool, nullptr);
}

void vren::imgui_renderer::render(
	uint32_t frame_idx,
	VkCommandBuffer cmd_buf,
	vren::resource_container& res_container,
	vren::render_target const& target,
	std::function<void()> const& show_guis_func
)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	show_guis_func();

	VkClearValue clear_values[] = {
		{},
		{ .depthStencil = { 1.0f, 0 } }
	};

	VkRenderPassBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.renderPass = m_render_pass.m_handle;
	begin_info.framebuffer = target.m_framebuffer;
	begin_info.renderArea = target.m_render_area;
	begin_info.clearValueCount = 2;
	begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd_buf, 0, 1, &target.m_viewport);
	vkCmdSetScissor(cmd_buf, 0, 1, &target.m_render_area);

	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf);

	vkCmdEndRenderPass(cmd_buf);
}
