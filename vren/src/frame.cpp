#include "frame.hpp"

#include "utils/image.hpp"
#include "descriptor_set_pool.hpp"

vren::frame::frame(
	std::shared_ptr<vren::context> const& ctx,
	VkImage image,
	VkImageView image_view,
	VkFramebuffer framebuffer
) :
	m_context(ctx)
{
	m_image = image;
	m_image_view = image_view;
	m_framebuffer = framebuffer;
}

vren::frame::frame(vren::frame&& other) noexcept :
	m_context(other.m_context)
{
	m_framebuffer = other.m_framebuffer;
	m_image_view  = other.m_image_view;
	m_image       = other.m_image;

	other.m_framebuffer = VK_NULL_HANDLE;
	other.m_image_view  = VK_NULL_HANDLE;
	other.m_image       = VK_NULL_HANDLE;
}

vren::frame::~frame()
{
	_release_descriptor_sets(); // todo auto-release to pool using vren::vk_descriptor_set lifetime?
}

void vren::frame::add_in_semaphore(std::shared_ptr<vren::vk_semaphore> const& semaphore)
{
	// The semaphore is added to a dedicated list for fast access and its lifetime is tracked like if it's any other resource.
	m_in_semaphores.push_back(semaphore->m_handle);
	track_resource(semaphore);
}

void vren::frame::add_out_semaphore(std::shared_ptr<vren::vk_semaphore> const& semaphore)
{
	m_out_semaphores.push_back(semaphore->m_handle);
	track_resource(semaphore);
}

void vren::frame::add_out_fence(std::shared_ptr<vren::vk_fence> const& fence)
{
	m_out_fences.push_back(fence->m_handle);
	track_resource(fence);
}

VkDescriptorSet vren::frame::acquire_material_descriptor_set()
{
	VkDescriptorSet descriptor_set;
	m_context->m_descriptor_set_pool->acquire_material_descriptor_set(&descriptor_set);

	m_acquired_descriptor_sets.push_back(descriptor_set);

	return descriptor_set; 
}

VkDescriptorSet vren::frame::acquire_lights_array_descriptor_set()
{
	VkDescriptorSet descriptor_set;
	m_context->m_descriptor_set_pool->acquire_lights_array_descriptor_set(&descriptor_set);

	m_acquired_descriptor_sets.push_back(descriptor_set);

	return descriptor_set;
}

void vren::frame::_release_descriptor_sets()
{
	if (!m_acquired_descriptor_sets.empty())
	{
		for (VkDescriptorSet descriptor_set : m_acquired_descriptor_sets)
		{
			m_context->m_descriptor_set_pool->release_descriptor_set(descriptor_set);
		}

		m_acquired_descriptor_sets.clear();
	}
}
