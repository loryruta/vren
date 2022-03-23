#pragma once

#include <vector>
#include <memory>
#include <iostream>

namespace vren
{
    // Forward decl
    template<typename _t>
    class object_pool;

    // --------------------------------------------------------------------------------------------------------------------------------
    // pooled_object
    // --------------------------------------------------------------------------------------------------------------------------------

    template<typename _t>
    class pooled_object
    {
    private:
        std::shared_ptr<object_pool<_t>> m_pool;
		_t m_handle;

    public:
        explicit pooled_object(std::shared_ptr<object_pool<_t>> const& pool, _t&& handle) :
            m_pool(pool),
            m_handle(std::move(handle))
        {}
        pooled_object(pooled_object<_t> const& other) = delete;
        pooled_object(pooled_object<_t>&& other) noexcept :
			m_pool(std::move(other.m_pool)),
			m_handle(std::move(other.m_handle))
        {
			other.m_pool = nullptr;
        }

        ~pooled_object()
        {
            if (m_pool) {
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

		_t const& get() const
		{
			return m_handle;
		}
    };

    // --------------------------------------------------------------------------------------------------------------------------------
    // object_pool
    // --------------------------------------------------------------------------------------------------------------------------------

    template<typename _t>
    class object_pool : public std::enable_shared_from_this<object_pool<_t>>
    {
        friend pooled_object<_t>;

	private:
		int m_acquired_count = 0;

    protected:
		std::vector<_t> m_unused_objects;

        virtual _t create_object() = 0;

        virtual void release(_t&& obj)
		{
            m_unused_objects.push_back(std::move(obj));
			m_acquired_count--;
        }

    public:
		object_pool() = default;
		~object_pool()
		{
			std::cout << "deleting pool" << std::endl;
		}

		int get_acquired_objects_count() const
		{
			return m_acquired_count;
		}

		int get_pooled_objects_count() const
		{
			return m_unused_objects.size();
		}

		int get_created_objects_count() const
		{
			return get_acquired_objects_count() + get_pooled_objects_count();
		}

        virtual vren::pooled_object<_t> acquire()
        {
			m_acquired_count++;

            if (m_unused_objects.empty()) {
                return vren::pooled_object(
                    std::enable_shared_from_this<object_pool<_t>>::shared_from_this(),
                    create_object()
                );
            } else {
                _t pooled_obj(std::move(m_unused_objects.back()));
                m_unused_objects.pop_back();

                return vren::pooled_object(
                    std::enable_shared_from_this<object_pool<_t>>::shared_from_this(),
                    std::move(pooled_obj)
                );
            }
        }
    };
}
