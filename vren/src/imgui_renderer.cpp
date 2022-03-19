#include "imgui_renderer.hpp"

#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>

#include "utils/misc.hpp"

// References:
// - https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
// - https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.h
// -https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_vulkan.cpp

struct ImGui_ImplVulkanH_FrameRenderBuffers
{
	VkDeviceMemory      VertexBufferMemory;
	VkDeviceMemory      IndexBufferMemory;
	VkDeviceSize        VertexBufferSize;
	VkDeviceSize        IndexBufferSize;
	VkBuffer            VertexBuffer;
	VkBuffer            IndexBuffer;
};

struct ImGui_ImplVulkanH_WindowRenderBuffers
{
	uint32_t            Index;
	uint32_t            Count;
	ImGui_ImplVulkanH_FrameRenderBuffers*   FrameRenderBuffers;
};

struct ImGui_ImplVulkan_Data
{
	ImGui_ImplVulkan_InitInfo   VulkanInitInfo;
	VkRenderPass                RenderPass;
	VkDeviceSize                BufferMemoryAlignment;
	VkPipelineCreateFlags       PipelineCreateFlags;
	VkDescriptorSetLayout       DescriptorSetLayout;
	VkPipelineLayout            PipelineLayout;
	VkPipeline                  Pipeline;
	uint32_t                    Subpass;
	VkShaderModule              ShaderModuleVert;
	VkShaderModule              ShaderModuleFrag;

	// Font data
	VkSampler                   FontSampler;
	VkDeviceMemory              FontMemory;
	VkImage                     FontImage;
	VkImageView                 FontView;
	VkDescriptorSet             FontDescriptorSet;
	VkDeviceMemory              UploadBufferMemory;
	VkBuffer                    UploadBuffer;

	// Render buffers
	ImGui_ImplVulkanH_WindowRenderBuffers MainWindowRenderBuffers;

	ImGui_ImplVulkan_Data()
	{
		memset(this, 0, sizeof(*this));
		BufferMemoryAlignment = 256;
	}
};

static ImGui_ImplVulkan_Data* ImGui_ImplVulkan_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplVulkan_Data*) ImGui::GetIO().BackendRendererUserData : NULL;
}

// --------------------------------------------------------------------------------------------------------------------------------
// imgui_descriptor_pool
// --------------------------------------------------------------------------------------------------------------------------------

vren::imgui_descriptor_pool::imgui_descriptor_pool(std::shared_ptr<vren::context> const& ctx) :
	vren::descriptor_pool(ctx)
{}

vren::imgui_descriptor_pool::~imgui_descriptor_pool()
{}

VkDescriptorPool vren::imgui_descriptor_pool::create_descriptor_pool(int max_sets)
{
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
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool desc_pool;
	vren::vk_utils::check(vkCreateDescriptorPool(m_context->m_device, &pool_info, nullptr, &desc_pool));

	return desc_pool;
}

vren::vk_descriptor_set vren::imgui_descriptor_pool::acquire_descriptor_set()
{
	ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
	return vren::descriptor_pool::acquire_descriptor_set(bd->DescriptorSetLayout);
}

void vren::imgui_descriptor_pool::write_descriptor_set(
	vren::vk_descriptor_set const& desc_set,
	VkSampler sampler,
	VkImageView image_view,
	VkImageLayout image_layout
)
{
	VkDescriptorImageInfo desc_image{};
	desc_image.sampler = sampler;
	desc_image.imageView = image_view;
	desc_image.imageLayout = image_layout;

	VkWriteDescriptorSet write_desc{};
	write_desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_desc.dstSet = desc_set.m_handle;
	write_desc.descriptorCount = 1;
	write_desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_desc.pImageInfo = &desc_image;
	vkUpdateDescriptorSets(m_context->m_device, 1, &write_desc, 0, NULL);
}

// --------------------------------------------------------------------------------------------------------------------------------
// imgui_renderer
// --------------------------------------------------------------------------------------------------------------------------------

vren::imgui_renderer::imgui_renderer(std::shared_ptr<vren::context> const& ctx, GLFWwindow* window) :
	m_context(ctx),
	m_window(window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = nullptr;

	ImPlot::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	m_descriptor_pool = std::make_shared<vren::imgui_descriptor_pool>(m_context);
	_init_render_pass();

	m_internal_descriptor_pool = m_descriptor_pool->create_descriptor_pool(1000);

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = ctx->m_instance;
	init_info.PhysicalDevice = ctx->m_physical_device;
	init_info.Device = ctx->m_device;
	init_info.QueueFamily = ctx->m_queue_families.m_graphics_idx;
	init_info.Queue = ctx->m_graphics_queue;
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = m_internal_descriptor_pool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 3; // Why do you need to know about image count here, dear imgui vulkan backend...?
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vren::vk_utils::check;
	ImGui_ImplVulkan_Init(&init_info, m_render_pass);

	vren::vk_utils::immediate_graphics_queue_submit(*ctx, [&](VkCommandBuffer cmd_buf, vren::resource_container& res_container) {
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

	vkDestroyRenderPass(m_context->m_device, m_render_pass, nullptr);
	vkDestroyDescriptorPool(m_context->m_device, m_internal_descriptor_pool, nullptr);
}

void vren::imgui_renderer::_init_render_pass()
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
	vren::vk_utils::check(vkCreateRenderPass(m_context->m_device, &render_pass_info, nullptr, &m_render_pass));
}

void vren::imgui_renderer::render(
	int frame_idx,
	vren::resource_container& res_container,
	vren::render_target const& target,
	std::function<void()> const& show_guis_func,
	uint32_t src_semaphores_count,
	VkSemaphore* src_semaphores,
	uint32_t dst_semaphore_count,
	VkSemaphore* dst_semaphores
)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	show_guis_func();

	auto cmd_buf = std::make_shared<vren::pooled_vk_command_buffer>(
		m_context->m_graphics_command_pool->acquire()
	);
	res_container.add_resource(cmd_buf);

	/* Command buffer begin */
	VkCommandBufferBeginInfo cmd_buf_begin_info{};
	cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_begin_info.pNext = nullptr;
	cmd_buf_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	cmd_buf_begin_info.pInheritanceInfo = nullptr;

	vren::vk_utils::check(vkBeginCommandBuffer(cmd_buf->m_handle, &cmd_buf_begin_info));

	/* Recording */
	VkClearValue clear_values[] = {
		{},
		{ .depthStencil = { 1.0f, 0 } }
	};

	VkRenderPassBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.renderPass = m_render_pass;
	begin_info.framebuffer = target.m_framebuffer;
	begin_info.renderArea = target.m_render_area;
	begin_info.clearValueCount = 2;
	begin_info.pClearValues = clear_values;

	vkCmdBeginRenderPass(cmd_buf->m_handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(cmd_buf->m_handle, 0, 1, &target.m_viewport);
	vkCmdSetScissor(cmd_buf->m_handle, 0, 1, &target.m_render_area);

	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmd_buf->m_handle);

	vkCmdEndRenderPass(cmd_buf->m_handle);

	/* Command buffer end */
	vren::vk_utils::check(vkEndCommandBuffer(cmd_buf->m_handle));

	/* Submission */
	std::vector<VkPipelineStageFlags> wait_stages(src_semaphores_count, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = src_semaphores_count;
	submit_info.pWaitSemaphores = src_semaphores;
	submit_info.pWaitDstStageMask = wait_stages.data();
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf->m_handle;
	submit_info.signalSemaphoreCount = dst_semaphore_count;
	submit_info.pSignalSemaphores = dst_semaphores;

	vren::vk_utils::check(vkQueueSubmit(m_context->m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
}
