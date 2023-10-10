#include "DeviceImage3d.hpp"

#include "Renderer.hpp"

using namespace explo;

DeviceImage3d::DeviceImage3d(Renderer &renderer, glm::ivec3 const &size, VkFormat format, VkMemoryPropertyFlags memory_properties) :
    m_renderer(renderer),
    m_size(size),
    m_format(format),
    m_memory_properties(memory_properties)
{
    m_image = std::make_shared<vren::vk_utils::combined_image_view>(create_image());
}

DeviceImage3d::~DeviceImage3d() {}

void DeviceImage3d::write_region(glm::ivec3 const &region_position, glm::ivec3 const &region_size, void *data, size_t data_size)
{
    std::shared_ptr<vren::vk_utils::buffer> staging_buffer = std::make_shared<vren::vk_utils::buffer>(vren::vk_utils::alloc_host_visible_buffer(
        m_renderer.m_context,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        data_size,
        true  // Persistently mapped
    ));

    vren::vk_utils::update_host_visible_buffer(m_renderer.m_context, *staging_buffer, data, data_size, 0);

    Op op{};
    op.m_type = OpType::OP_WRITE_REGION;

    WriteRegionOp &write_region_op = op.m_write_region;
    write_region_op.m_buffer = staging_buffer;
    write_region_op.m_buffer_offset = 0;
    write_region_op.m_region_position = region_position;
    write_region_op.m_region_size = region_size;

    m_operations.push_back(op);
}

void DeviceImage3d::write_pixel(glm::ivec3 const &position, void *data, size_t data_size)
{
    write_region(position, glm::ivec3{1, 1, 1}, data, data_size);
}

void DeviceImage3d::record(VkCommandBuffer command_buffer, vren::resource_container &resource_container)
{
    for (Op const &op : m_operations)
    {
        switch (op.m_type)
        {
            case OP_WRITE_REGION:
                perform_write_region(command_buffer, resource_container, op.m_write_region);
                break;
            default:
                throw std::runtime_error("Invalid operation type");
        }
    }

    m_operations.clear();
}

void DeviceImage3d::perform_write_region(
    VkCommandBuffer command_buffer, vren::resource_container &resource_container, WriteRegionOp const &write_region_op
)
{
    glm::ivec3 reg_pos = write_region_op.m_region_position;
    glm::ivec3 reg_size = write_region_op.m_region_size;

    VkBufferImageCopy buffer_image_copy{};
    buffer_image_copy.bufferOffset = write_region_op.m_buffer_offset;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset = {reg_pos.x, reg_pos.y, reg_pos.z};
    buffer_image_copy.imageExtent = {uint32_t(reg_size.x), uint32_t(reg_size.y), uint32_t(reg_size.z)};

    transit_image_layout(command_buffer, m_image->m_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkCmdCopyBufferToImage(
        command_buffer,
        write_region_op.m_buffer->m_buffer.m_handle,
        m_image->m_image.m_image.m_handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &buffer_image_copy
    );

    transit_image_layout(command_buffer, m_image->m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    resource_container.add_resources(write_region_op.m_buffer, m_image);
}

vren::vk_utils::combined_image_view DeviceImage3d::create_image()
{
    // Creates the image
    VkImageCreateInfo image_info{};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.flags = NULL;
    image_info.imageType = VK_IMAGE_TYPE_3D;
    image_info.format = m_format;
    image_info.extent = {uint32_t(m_size.x), uint32_t(m_size.y), uint32_t(m_size.z)};
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_create_info{};
    alloc_create_info.requiredFlags = m_memory_properties;

    VkImage image_handle{};
    VmaAllocation alloc{};
    VmaAllocationInfo alloc_info{};
    vmaCreateImage(m_renderer.m_context.m_vma_allocator, &image_info, &alloc_create_info, &image_handle, &alloc, &alloc_info);

    vren::vk_utils::image image = vren::vk_utils::image{
        .m_image = vren::vk_image(m_renderer.m_context, image_handle),
        .m_allocation = vren::vma_allocation(m_renderer.m_context, alloc),
        .m_allocation_info = alloc_info};

    // Immediately transits the image to VK_IMAGE_LAYOUT_GENERAL
    vren::vk_utils::immediate_graphics_queue_submit(
        m_renderer.m_context,
        [&](VkCommandBuffer command_buffer, vren::resource_container &resource_container)
        {
            vren::vk_utils::transit_image_layout(command_buffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        }
    );

    // Creates the image view
    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.pNext = nullptr;
    image_view_info.flags = NULL;
    image_view_info.image = image.m_image.m_handle;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
    image_view_info.format = m_format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    VkImageView image_view_handle{};
    vkCreateImageView(m_renderer.m_context.m_device, &image_view_info, nullptr, &image_view_handle);

    vren::vk_image_view image_view = vren::vk_image_view(m_renderer.m_context, image_view_handle);

    // Combines them into an image/image_view struct
    return {
        .m_image = std::move(image),
        .m_image_view = std::move(image_view),
    };
}
