#pragma once

#include <vulkan/vulkan.h>

#include "object_pool.hpp"
#include "vk_helpers/vk_raii.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// Fence pool
	// ------------------------------------------------------------------------------------------------

	using pooled_vk_fence = vren::pooled_object<vren::vk_fence>;

    class fence_pool : public vren::object_pool<vren::vk_fence>
    {
    private:
		vren::context const* m_context;

    public:
		explicit fence_pool(vren::context const& ctx);

        vren::pooled_vk_fence acquire();
    };
}
