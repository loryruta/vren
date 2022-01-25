#include "debug_gui.hpp"

#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>

#include "renderer.hpp"
#include "presenter.hpp"
#include "vk_utils.hpp"

// Reference:
// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp

vren::debug_gui::debug_gui(vren::renderer& renderer, GLFWwindow* window) :
	m_renderer(renderer),
	m_window(window)
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

	vren::vk_utils::check(vkCreateDescriptorPool(renderer.m_device, &pool_info, nullptr, &m_descriptor_pool));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;

	ImPlot::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = renderer.m_instance;
	init_info.PhysicalDevice = renderer.m_physical_device;
	init_info.Device = renderer.m_device;
	init_info.QueueFamily = renderer.m_queue_families.m_graphics_idx;
	init_info.Queue = renderer.m_graphics_queue;
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = m_descriptor_pool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vren::vk_utils::check;
	ImGui_ImplVulkan_Init(&init_info, renderer.m_render_pass);

	// Font uploading
	vren::vk_utils::immediate_submit(renderer, [&](VkCommandBuffer cmd_buf) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd_buf);
	});

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

vren::debug_gui::~debug_gui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(m_renderer.m_device, m_descriptor_pool, nullptr);
}

void vren::debug_gui::render(vren::frame& frame)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, frame.m_command_buffer);
}

