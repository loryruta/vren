#include "frame.hpp"

#include "renderer.hpp"

vren::frame::frame(vren::renderer& renderer) :
	m_renderer(renderer)
{
	{ // Command buffer
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = m_renderer.m_graphics_command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		vren::vk_utils::check(vkAllocateCommandBuffers(m_renderer.m_device, &alloc_info, &m_command_buffer));
	}

	{ // Image available semaphore
		VkSemaphoreCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		vren::vk_utils::check(vkCreateSemaphore(m_renderer.m_device, &create_info, nullptr, &m_image_available_semaphore));
	}

	{ // Render finished semaphore
		VkSemaphoreCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		vren::vk_utils::check(vkCreateSemaphore(m_renderer.m_device, &create_info, nullptr, &m_render_finished_semaphore));
	}

	{ // Render finished fence
		VkFenceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		vren::vk_utils::check(vkCreateFence(m_renderer.m_device, &create_info, nullptr, &m_render_finished_fence));
	}
}

vren::frame::frame(vren::frame&& other) noexcept :
	m_renderer(other.m_renderer)
{
	m_swapchain_framebuffer = other.m_swapchain_framebuffer;
	m_swapchain_image_view  = other.m_swapchain_image_view;
	m_swapchain_image       = other.m_swapchain_image;

	m_command_buffer = other.m_command_buffer;

	m_image_available_semaphore = other.m_image_available_semaphore;
	m_render_finished_semaphore = other.m_render_finished_semaphore;
	m_render_finished_fence     = other.m_render_finished_fence;

	other.m_swapchain_framebuffer = VK_NULL_HANDLE;
	other.m_swapchain_image_view  = VK_NULL_HANDLE;
	other.m_swapchain_image       = VK_NULL_HANDLE;

	other.m_command_buffer = VK_NULL_HANDLE;

	other.m_image_available_semaphore = VK_NULL_HANDLE;
	other.m_render_finished_semaphore = VK_NULL_HANDLE;
	other.m_render_finished_fence     = VK_NULL_HANDLE;
}

vren::frame::~frame()
{
	if (m_command_buffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(m_renderer.m_device, m_renderer.m_graphics_command_pool, 1, &m_command_buffer);
	}

	if (m_image_available_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(m_renderer.m_device, m_image_available_semaphore, nullptr);
	if (m_render_finished_semaphore != VK_NULL_HANDLE) vkDestroySemaphore(m_renderer.m_device, m_render_finished_semaphore, nullptr);
	if (m_render_finished_fence     != VK_NULL_HANDLE) vkDestroyFence(m_renderer.m_device, m_render_finished_fence, nullptr);
}

void vren::frame::_release_descriptor_sets()
{
	if (!m_acquired_descriptor_sets.empty())
	{
		for (VkDescriptorSet descriptor_set : m_acquired_descriptor_sets)
		{
			m_renderer.m_descriptor_set_pool->release_descriptor_set(descriptor_set);
		}

		m_acquired_descriptor_sets.clear();
	}
}

void vren::frame::_on_render()
{
	_release_descriptor_sets();
}

VkDescriptorSet vren::frame::acquire_material_descriptor_set()
{
	VkDescriptorSet descriptor_set;
	m_renderer.m_descriptor_set_pool->acquire_material_descriptor_set(&descriptor_set);

	m_acquired_descriptor_sets.push_back(descriptor_set);

	return descriptor_set; 
}

VkDescriptorSet vren::frame::acquire_lights_array_descriptor_set()
{
	VkDescriptorSet descriptor_set;
	m_renderer.m_descriptor_set_pool->acquire_lights_array_descriptor_set(&descriptor_set);

	m_acquired_descriptor_sets.push_back(descriptor_set);

	return descriptor_set;
}
