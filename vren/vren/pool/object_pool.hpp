#pragma once

#include <optional>
#include <vector>

namespace vren
{
    // Forward decl
    template <typename _t> class object_pool;

    // ------------------------------------------------------------------------------------------------
    // Pooled object
    // ------------------------------------------------------------------------------------------------

    template <typename _t> class pooled_object
    {
    private:
        object_pool<_t>* m_pool;

    public:
        _t m_handle;

        explicit pooled_object(object_pool<_t>& pool, _t&& handle) :
            m_pool(&pool),
            m_handle(std::move(handle))
        {
        }
        pooled_object(pooled_object<_t> const& other) = delete;
        pooled_object(pooled_object<_t>&& other) noexcept :
            m_pool(other.m_pool),
            m_handle(std::move(other.m_handle))
        {
            other.m_pool = nullptr;
        }

        ~pooled_object()
        {
            if (m_pool)
            {
                m_pool->release(std::move(m_handle));
            }
        }

        pooled_object<_t>& operator=(pooled_object<_t> const& other) = delete;
        pooled_object<_t>& operator=(pooled_object<_t>&& other) noexcept
        {
            m_pool = std::move(other.m_pool);
            m_handle = std::move(other.m_handle);

            other.m_pool = nullptr;

            return *this;
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Object pool
    // ------------------------------------------------------------------------------------------------

    template <typename _t> class object_pool
    {
        friend pooled_object<_t>;

    private:
        int m_acquired_count = 0;

    protected:
        std::vector<_t> m_unused_objects;

        explicit object_pool() = default;

        virtual void release(_t&& obj)
        {
            m_unused_objects.push_back(std::move(obj));
            m_acquired_count--;
        }

        vren::pooled_object<_t> create_managed_object(_t&& obj) { return vren::pooled_object(*this, std::move(obj)); }

    public:
        object_pool(object_pool<_t> const& other) = delete;
        object_pool(object_pool<_t>&& other) = delete;

        ~object_pool() = default;

        int get_acquired_objects_count() const
        {
            return 0; // TODO
        }

        int get_pooled_objects_count() const { return m_unused_objects.size(); }

        int get_created_objects_count() const { return get_acquired_objects_count() + get_pooled_objects_count(); }

        std::optional<vren::pooled_object<_t>> try_acquire()
        {
            if (m_unused_objects.size() > 0)
            {
                _t pooled_obj(std::move(m_unused_objects.back()));
                m_unused_objects.pop_back();

                return create_managed_object(std::move(pooled_obj));
            }

            return std::nullopt;
        }
    };
} // namespace vren
