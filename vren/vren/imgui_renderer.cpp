#include "imgui_renderer.hpp"

#include <volk.h>
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_vulkan.h>

#include "context.hpp"
#include "vk_helpers/misc.hpp"

vren::imgui_renderer::imgui_renderer(vren::context const& context, imgui_windowing_backend_hooks const& windowing_backend_hooks) :
	m_context(&context),
	m_windowing_backend_hooks(windowing_backend_hooks),
	m_descriptor_pool(create_descriptor_pool()),
	m_render_pass(create_render_pass())
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = nullptr;

	ImPlot::CreateContext();

	// Use volk
	ImGui_ImplVulkan_LoadFunctions([](char const* func_name, void* user_data) {
		return (PFN_vkVoidFunction) vkGetInstanceProcAddr((VkInstance) user_data, func_name);
	}, context.m_instance);

	m_windowing_backend_hooks.m_init_callback(); // Example: ImGui_ImplGlfw_InitForVulkan

	// Init Vulkan
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = context.m_instance;
	init_info.PhysicalDevice = context.m_physical_device;
	init_info.Device = context.m_device;
	init_info.QueueFamily = context.m_queue_families.m_graphics_idx;
	init_info.Queue = context.m_graphics_queue;
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = m_descriptor_pool.m_handle;
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
	m_descriptor_pool.~vk_descriptor_pool(); // Manually call destructor to ensure these elements are destroyed before the ImGui context
	m_render_pass.~vk_render_pass();

	// Shutdown Vulkan
	ImGui_ImplVulkan_Shutdown();

	m_windowing_backend_hooks.m_shutdown_callback(); // Example: ImGui_ImplGlfw_Shutdown

	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

vren::vk_descriptor_pool vren::imgui_renderer::create_descriptor_pool()
{
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1024 }
	};
	VkDescriptorPoolCreateInfo descriptor_pool_info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 512,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	VkDescriptorPool descriptor_pool;
	VREN_CHECK(vkCreateDescriptorPool(m_context->m_device, &descriptor_pool_info, nullptr, &descriptor_pool), m_context);
	return vren::vk_descriptor_pool(*m_context, descriptor_pool);
}

vren::vk_render_pass vren::imgui_renderer::create_render_pass()
{
	VkAttachmentDescription color_attachment{
		.format = VREN_COLOR_BUFFER_OUTPUT_FORMAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference color_attachment_ref{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass{
		.flags = NULL,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	VkRenderPassCreateInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = NULL,
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 0, // Dependencies are expressed through barriers placed by the render-graph
		.pDependencies = nullptr
	};

	VkRenderPass render_pass;
	VREN_CHECK(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &render_pass), m_context);
	return vren::vk_render_pass(*m_context, render_pass);
}

vren::render_graph_node* vren::imgui_renderer::render(vren::render_target const& render_target, std::function<void()> const& show_ui_callback)
{
	auto node = new vren::render_graph_node();
	node->set_name("debug_renderer | render");
	node->set_src_stage(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
	node->set_dst_stage(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
	node->add_image(
		{.m_name = "color_buffer", .m_image = render_target.m_color_buffer->get_image(), .m_image_aspect = VK_IMAGE_ASPECT_COLOR_BIT},
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	);
	node->set_callback([=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
	{
		// Create UIs
		ImGui_ImplVulkan_NewFrame();
		m_windowing_backend_hooks.m_new_frame_callback(); // Example: ImGui_ImplGlfw_NewFrame()
		ImGui::NewFrame();

		show_ui_callback();

		// Create a temporary framebuffer (to emulate dynamic rendering feature since ImGui Vulkan backend doesn't support it)
		VkImageView attachments[] { render_target.m_color_buffer->get_image_view() };
		auto framebuffer_size = render_target.m_render_area.extent;
		auto framebuffer = std::make_shared<vren::vk_framebuffer>(
			vren::vk_utils::create_framebuffer(*m_context, m_render_pass.m_handle, framebuffer_size.width, framebuffer_size.height, attachments)
		);
		resource_container.add_resource(framebuffer);

		// Render-pass begin
		VkRenderPassBeginInfo render_pass_begin_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = m_render_pass.m_handle,
			.framebuffer = framebuffer->m_handle,
			.renderArea = render_target.m_render_area,
			.clearValueCount = 0,
			.pClearValues = nullptr
		};
		vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(command_buffer, 0, 1, &render_target.m_viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &render_target.m_render_area);

		// Recording
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);

		// Render-pass end
		vkCmdEndRenderPass(command_buffer);
	});
	return node;
}
