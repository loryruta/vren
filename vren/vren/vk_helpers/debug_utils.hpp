#pragma once

#include <volk.h>

#include "misc.hpp"
#include "context.hpp"

namespace vren::vk_utils
{
	inline void set_object_name(vren::context const& context, VkObjectType object_type, uint64_t object_handle, char const* object_name)
	{
		VkDebugUtilsObjectNameInfoEXT obj_name_info{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = object_type,
			.objectHandle = object_handle,
			.pObjectName = object_name
		};
		VREN_CHECK(vkSetDebugUtilsObjectNameEXT(context.m_device, &obj_name_info), &context);
	}
}

