#pragma once

#include "volk.h"

#include "Context.hpp"
#include "misc_utils.hpp"
#include "shader.hpp"
#include "vren/vk_api/buffer/buffer.hpp"
#include "vren/vk_api/image/image.hpp"

namespace vren::vk_utils
{
    inline void set_object_name(
        vren::context const& context, VkObjectType object_type, uint64_t object_handle, char const* object_name
    )
    {
        VkDebugUtilsObjectNameInfoEXT obj_name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = object_type,
            .objectHandle = object_handle,
            .pObjectName = object_name};
        VREN_CHECK(vkSetDebugUtilsObjectNameEXT(context.m_device, &obj_name_info), &context);
    }

    inline void set_name(
        vren::context const& context, vren::vk_utils::combined_image_view const& combined_image_view, char const* name
    )
    {
        std::string image_name = std::string(name) + "_image";
        std::string image_view_name = std::string(name) + "_image_view";

        vren::vk_utils::set_object_name(
            context,
            VK_OBJECT_TYPE_IMAGE,
            reinterpret_cast<uint64_t>(combined_image_view.get_image()),
            image_name.c_str()
        );
        vren::vk_utils::set_object_name(
            context,
            VK_OBJECT_TYPE_IMAGE_VIEW,
            reinterpret_cast<uint64_t>(combined_image_view.get_image_view()),
            image_view_name.c_str()
        );
    }

    inline void set_name(vren::context const& context, vren::vk_utils::buffer const& buffer, char const* name)
    {
        vren::vk_utils::set_object_name(
            context, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer.m_buffer.m_handle), name
        );
    }

    inline void set_name(vren::context const& context, vren::shader_module const& shader_module, char const* name)
    {
        vren::vk_utils::set_object_name(
            context, VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64_t>(shader_module.m_handle.m_handle), name
        );
    }

    inline void set_name(vren::context const& context, vren::pipeline const& pipeline, char const* name)
    {
        vren::vk_utils::set_object_name(
            context, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(pipeline.m_handle.m_handle), name
        );
    }

    inline void set_name(vren::context const& context, VkRenderPass render_pass, char const* name)
    {
        vren::vk_utils::set_object_name(
            context, VK_OBJECT_TYPE_RENDER_PASS, reinterpret_cast<uint64_t>(render_pass), name
        );
    }
} // namespace vren::vk_utils
