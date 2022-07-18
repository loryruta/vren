#pragma once

#include "vk_helpers/buffer.hpp"
#include "vk_helpers/shader.hpp"

namespace vren
{
    class reduce_data_type
    {
    public:
        enum enum_t
        {
            Uint,
            Float,
            Vec3,
        };

        inline static char const* get_name(vren::reduce_data_type::enum_t data_type)
        {
            switch (data_type)
            {
            case Uint:  return "uint";
            case Float: return "float";
            case Vec3:  return "vec3";
            }
        }
    };

    class reduce_operation
    {
    public:
        enum enum_t
        {
            Add,
            Min,
            Max,
        };

        inline static char const* get_name(vren::reduce_operation::enum_t data_type)
        {
            switch (data_type)
            {
            case Add: return "add";
            case Min: return "min";
            case Max: return "max";
            }
        }
    };

    class reduce
    {
    public:
        inline static const uint32_t k_workgroup_size = 1024;

    private:
        vren::context const* m_context;
        vren::pipeline m_pipeline;

    public:
        reduce(
            vren::context const& context,
            vren::reduce_data_type::enum_t data_type,
            vren::reduce_operation::enum_t operation
        );

    private:
        void write_descriptor_set(
            VkDescriptorSet descriptor_set,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset
        );

    public:
        void operator()(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            vren::vk_utils::buffer const& buffer,
            uint32_t length,
            uint32_t offset,
            uint32_t blocks_num,
            uint32_t* result = nullptr
        );
    };
}
