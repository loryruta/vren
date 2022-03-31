#pragma once

#include <vulkan/vulkan.h>

#include "object_pool.hpp"
#include "utils/vk_raii.hpp"

namespace vren
{
	// Forward decl
	class context;

	// ------------------------------------------------------------------------------------------------
	// fence_pool
	// ------------------------------------------------------------------------------------------------

	using pooled_vk_fence = vren::pooled_object<vren::vk_fence>;

    class fence_pool : public vren::object_pool<vren::vk_fence>
    {
    private:
        std::shared_ptr<vren::context> m_context;

    protected:
		explicit fence_pool(std::shared_ptr<vren::context> ctx);

    public:
        vren::pooled_vk_fence acquire();

		static std::shared_ptr<fence_pool> create(std::shared_ptr<vren::context> const& ctx);
    };
}
