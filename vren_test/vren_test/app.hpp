#pragma once

#include <memory>

#include <vren/context.hpp>
#include <vren/pipeline/profiler.hpp>

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

        inline app() :
            m_context(m_context_info),
            m_profiler(m_context)
        {
        }

        static inline app* get()
        {
            static app singleton{};
            return &singleton;
        }
    };
}
