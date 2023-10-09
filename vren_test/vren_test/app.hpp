#pragma once

#include <memory>
#include <random>
#include <iterator>
#include <algorithm>
#include <string>

#include <vren/Context.hpp>
#include <vren/pipeline/profiler.hpp>
#include <vren/primitives/blelloch_scan.hpp>
#include <vren/primitives/reduce.hpp>
#include <vren/vk_helpers/buffer.hpp>
#include <vren/vk_helpers/misc.hpp>

#define VREN_TEST_APP() vren_test::app::get()

namespace vren_test
{
    class app
    {
    public:
        vren::context_info m_context_info{
            .m_app_name = "vren_test"
        };
        vren::context m_context;
        vren::profiler m_profiler;

        vren::blelloch_scan m_blelloch_scan;

        inline app() :
            m_context(m_context_info),
            m_profiler(m_context),
            m_blelloch_scan(m_context)
        {
        }

        static inline app* get()
        {
            static app singleton{};
            return &singleton;
        }
    };

    template<typename _t>
    void print_buffer(_t* buffer, size_t length, char const* format = "{}")
    {
        for (uint32_t i = 0; i < length; i++)
        {
            fmt::print(fmt::fg(fmt::color::dark_slate_gray), "({}) ", i);
            fmt::print(fmt::fg(buffer[i] == 0 ? fmt::color::dark_slate_gray : fmt::color::yellow), std::string(format) + ", ", buffer[i]);
        }

        printf("\n");
    }

    template<typename _t>
    void print_gpu_buffer(vren::context const& context, vren::vk_utils::buffer const& buffer, uint32_t length, char const* format = "{}")
    {
        size_t size = length * sizeof(_t);

        vren::vk_utils::buffer staging_buffer =
            vren::vk_utils::alloc_host_only_buffer(context, VK_BUFFER_USAGE_TRANSFER_DST_BIT, size, true);

        vren::vk_utils::immediate_graphics_queue_submit(context, [&](VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
        {
            VkBufferCopy region{
                .srcOffset = 0,
                .dstOffset = 0,
                .size = size,
            };
            vkCmdCopyBuffer(command_buffer, buffer.m_buffer.m_handle, staging_buffer.m_buffer.m_handle, 1, &region);
        });

        print_buffer<_t>(reinterpret_cast<_t*>(staging_buffer.m_allocation_info.pMappedData), size / sizeof(_t), format);
    }

    template<typename _iter_t, typename _int_t>
    void fill_with_random_int_values(_iter_t start, _iter_t end, _int_t min, _int_t max)
    {
        static std::random_device rd;
        static std::mt19937 mte(rd());

        std::uniform_int_distribution<_int_t> dist(min, max);
        std::generate(start, end, [&]() { return dist(mte); });
    }
}
