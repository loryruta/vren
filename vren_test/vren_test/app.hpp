#pragma once

#include <memory>

#include <vren/context.hpp>
#include <vren/pipeline/profiler.hpp>
#include <vren/primitive/reduce.hpp>
#include <vren/primitive/blelloch_scan.hpp>

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

        vren::reduce m_reduce;
        vren::blelloch_scan m_blelloch_scan;

        inline app() :
            m_context(m_context_info),
            m_profiler(m_context),
            m_reduce(m_context),
            m_blelloch_scan(m_context)
        {
        }

        static inline app* get()
        {
            static app singleton{};
            return &singleton;
        }
    };
}
