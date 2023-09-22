#pragma once

#include <vector>

#include <volk.h>

#include "object_pool.hpp"
#include "vk_helpers/vk_raii.hpp"

namespace vren
{
    // Forward decl
    class context;

    // ------------------------------------------------------------------------------------------------
    // Command pool
    // ------------------------------------------------------------------------------------------------

    using pooled_vk_command_buffer = pooled_object<VkCommandBuffer>;

    // TODO Doubt: does this class make any sense? Aren't we creating a pool of a pool?
    class command_pool : public vren::object_pool<VkCommandBuffer>
    {
    private:
        vren::context const* m_context;

    public:
        vren::vk_command_pool m_command_pool;

        explicit command_pool(vren::context const& ctx, vren::vk_command_pool&& cmd_pool);
        ~command_pool();

        pooled_vk_command_buffer acquire();
    };
} // namespace vren
