#pragma once

#include <volk.h>

#include "misc.hpp"
#include "context.hpp"

namespace vren::vk_utils
{
	void set_object_name(vren::context const& ctx, VkObjectType obj_type, uint64_t obj_handle, char const* obj_name)
	{
		VkDebugUtilsObjectNameInfoEXT obj_name_info{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = obj_type,
			.objectHandle = obj_handle,
			.pObjectName = obj_name
		};
		vren::vk_utils::check(vkSetDebugUtilsObjectNameEXT(ctx.m_device, &obj_name_info));
	}
}

