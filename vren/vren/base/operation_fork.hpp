#pragma once

#include <array>
#include <functional>

#include "base/base.hpp"

namespace vren
{
    template<typename _t, size_t _n, size_t _queue_length = 16>
    class operation_fork
    {
    public:
        using operation_t = std::function<void(_t& object)>;

    private:
        std::array<vren::static_vector_t<operation_t, _queue_length>, _n> m_operations;

    public:
        void enqueue(operation_t const& operation)
        {
            for (uint32_t i = 0; i < _n; i++)
            {
                m_operations[i].emplace_back(operation);
            }
        }

        void apply(uint32_t i, _t& object)
        {
            assert(i < _n);

            for (operation_t const& operation : m_operations.at(i))
            {
                operation(object);
            }

            m_operations[i].clear();
        }
    };
}
