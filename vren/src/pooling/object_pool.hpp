#pragma once

#include <vector>
#include <memory>

namespace vren
{
    // Forward decl
    template<typename _t>
    class object_pool;

    // --------------------------------------------------------------------------------------------------------------------------------
    // Pooled object
    // --------------------------------------------------------------------------------------------------------------------------------

    template<typename _t>
    class pooled_object
    {
    private:
        std::shared_ptr<object_pool<_t>> m_pool;

    public:
        _t m_handle;

        explicit pooled_object(std::shared_ptr<object_pool<_t>> const& pool, _t handle) :
            m_pool(pool),
            m_handle(handle)
        {}
        pooled_object(pooled_object<_t> const& other) = delete;
        pooled_object(pooled_object<_t>&& other) noexcept
        {
			*this = std::move(other);
        }

        ~pooled_object()
        {
            if (m_pool) {
                m_pool->release(m_handle);
            }
        }

        pooled_object<_t>& operator=(pooled_object<_t> const& other) = delete;
        pooled_object<_t>& operator=(pooled_object<_t>&& other) noexcept
        {
			m_pool = other.m_pool;
			m_handle = other.m_handle;

			other.m_pool = nullptr;

			return *this;
        }
    };

    // --------------------------------------------------------------------------------------------------------------------------------
    // Object pool
    // --------------------------------------------------------------------------------------------------------------------------------

    template<typename _t>
    class object_pool : public std::enable_shared_from_this<object_pool<_t>>
    {
        friend pooled_object<_t>;

    protected:
        virtual _t create_object() = 0;

        virtual void release(_t const& obj)
        {
            m_unused_objects.push_back(obj);
        }

    public:
        std::vector<_t> m_unused_objects;

        virtual vren::pooled_object<_t> acquire()
        {
            if (m_unused_objects.empty()) {
                return vren::pooled_object(
                    std::enable_shared_from_this<object_pool<_t>>::shared_from_this(),
                    create_object()
                );
            } else {
                auto pooled_obj = m_unused_objects.back();
                m_unused_objects.pop_back();
                return vren::pooled_object(
                    std::enable_shared_from_this<object_pool<_t>>::shared_from_this(),
                    pooled_obj
                );
            }
        }
    };
}
