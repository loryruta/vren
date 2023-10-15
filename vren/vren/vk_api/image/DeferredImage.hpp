#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/image.hpp>

namespace explo
{
    // Forward decl
    class Renderer;

    class DeviceImage3d
    {
    public:
        struct WriteRegionOp
        {
            std::shared_ptr<Buffer> m_buffer;
            size_t m_buffer_offset;
            glm::ivec3 m_region_position;
            glm::ivec3 m_region_size;
        };

        enum OpType : uint32_t
        {
            OP_WRITE_REGION
        };

        struct Op
        {
            OpType m_type;

            WriteRegionOp m_write_region;  // OP_WRITE_REGION
        };

    private:
        Renderer &m_renderer;

        std::shared_ptr<CombinedImageView> m_image;
        glm::ivec3 m_size;
        VkFormat m_format;
        VkMemoryPropertyFlags m_memory_properties;

        std::vector<Op> m_operations;

    public:
        explicit DeviceImage3d(Renderer &renderer, glm::ivec3 const &size, VkFormat format, VkMemoryPropertyFlags memory_properties);
        ~DeviceImage3d();

        std::shared_ptr<vren::vk_utils::combined_image_view> get_image() const { return m_image; }

        void write_pixel(glm::ivec3 const &position, void *data, size_t data_size);

        void write_region(glm::ivec3 const &region_position, glm::ivec3 const &region_size, void *data, size_t data_size);

        void record(VkCommandBuffer command_buffer, vren::resource_container &resource_container);

    private:
        void perform_write_region(VkCommandBuffer command_buffer, vren::resource_container &resource_container, WriteRegionOp const &write_region_op);

        vren::vk_utils::combined_image_view create_image();
    };
}  // namespace explo
