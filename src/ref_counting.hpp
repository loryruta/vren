#pragma once

#include <memory>
#include <utility>
#include <unordered_map>
#include <functional>

namespace vren
{
	struct _ref_counted_base
	{
	protected:
		virtual void* _get_resource() = 0;

	public:
		int m_uses;

		_ref_counted_base() :
			m_uses(0)
		{}

		~_ref_counted_base()
		{}

		template<typename _t>
		_t* get_resource()
		{
			return reinterpret_cast<_t*>(_get_resource());
		}
	};

	template<typename _t>
	class _ref_counted : public _ref_counted_base
	{
	private:
		_t m_res;

	protected:
		void* _get_resource() override
		{
			return &m_res;
		}

	public:
		template<class... _args_t>
		_ref_counted(_args_t&&... args) :
			m_res(std::forward<_args_t>(args)...)
		{}

		~_ref_counted()
		{}
	};

	class _ref_counted_deleter
	{
	private:
		template<typename _t>
		struct empty_arg_t {};

		struct concept_t
		{
			virtual void delete_(_ref_counted_base* ptr) = 0;
		};

		template<typename _t>
		struct model_t : public concept_t
		{
			void delete_(_ref_counted_base* ptr) override
			{
				delete dynamic_cast<_ref_counted<_t>*>(ptr);
			}
		};

		std::unique_ptr<concept_t> m_concept;

		template<typename _t>
		_ref_counted_deleter(empty_arg_t<_t>) :
			m_concept(std::make_unique<model_t<_t>>())
		{}

	public:
		void operator()(_ref_counted_base* ptr)
		{
			m_concept->delete_(ptr);
		}

		template<typename _t>
		static _ref_counted_deleter new_()
		{
			return _ref_counted_deleter(empty_arg_t<_t>{});
		}
	};

	class resource_container;

	template<typename _t>
	class rc
	{
	private:
		resource_container* m_container;
		int m_res_idx;
		std::shared_ptr<bool> m_is_container_alive;

		void _if_container_alive(std::function<void(resource_container&)> const& func)
		{
			if (is_valid())
			{
				func(*m_container);
			}
		}

		void _inc_uses()
		{
			_if_container_alive([&](resource_container& container) {
				container._get_control_block(m_res_idx)->m_uses++;
			});
		}

		void _dec_uses()
		{
			_if_container_alive([&](resource_container& container) {
				_ref_counted_base* control_block = m_container->_get_control_block(m_res_idx);
				control_block->m_uses--;
				if (control_block->m_uses == 0) {
					m_container->_destroy_resource_at(m_res_idx);
				}
			});
		}

	public:
		rc() :
			m_container(nullptr),
			m_res_idx(-1),
			m_is_container_alive(nullptr)
		{}

		rc(resource_container* container, int res_idx, std::shared_ptr<bool> is_container_alive) :
			m_container(container),
			m_res_idx(res_idx),
			m_is_container_alive(is_container_alive)
		{
			_inc_uses();
		}

		rc(rc const& other) :
			m_container(other.m_container),
			m_res_idx(other.m_res_idx),
			m_is_container_alive(other.m_is_container_alive)
		{
			_inc_uses();
		}

		rc(rc&& other) :
			m_container(other.m_container),
			m_res_idx(other.m_res_idx),
			m_is_container_alive(other.m_is_container_alive)
		{
			other.m_container = nullptr;
			other.m_res_idx = -1;
			other.m_is_container_alive = nullptr;
		}

		~rc()
		{
			_dec_uses();
		}

		void swap(rc& other)
		{
			std::swap(m_container, other.m_container);
			std::swap(m_res_idx, other.m_res_idx);
			std::swap(m_is_container_alive, other.m_is_container_alive);
		}

		bool is_valid() const
		{
			return m_container && m_is_container_alive && *m_is_container_alive && m_res_idx >= 0;
		}

		void reset()
		{
			rc().swap(*this);
		}

		int use_count() const
		{
			return is_valid() ? m_container->_get_control_block(m_res_idx)->m_uses : 0;
		}

		_t* get() const
		{
			if (!is_valid())
			{
				throw std::runtime_error("Container must be alive to perform this action");
			}

			return m_container->_get_control_block(m_res_idx)->template get_resource<_t>();
		}

		_t& operator*() const
		{
			return *get();
		}

		_t* operator->() const
		{
			return get();
		}

		rc& operator=(rc const& other)
		{
			rc(other).swap(*this);
			return *this;
		}

		rc& operator=(rc&& other)
		{
			rc(std::move(other)).swap(*this);
			return *this;
		}

		operator bool() const
		{
			return is_valid();
		}
	};

	class resource_container
	{
	private:
		std::unordered_map<int, std::unique_ptr<_ref_counted_base, _ref_counted_deleter>> m_control_block_by_res_idx;
		int m_next_res_idx;

		std::shared_ptr<bool> m_is_alive;

	public:
		explicit inline resource_container() :
			m_is_alive(std::make_shared<bool>(true)),
			m_next_res_idx(0)
		{}

		inline ~resource_container()
		{
			release();
		}

		void release()
		{
			*m_is_alive = false;

			printf("INFO: Deleting %zu resources\n", get_resources_count());
			fflush(stdout);

			m_control_block_by_res_idx.clear();
		}

		_ref_counted_base* _get_control_block(int res_idx)
		{
			return m_control_block_by_res_idx.at(res_idx).get();
		}

		void _destroy_resource_at(int res_idx)
		{
			m_control_block_by_res_idx.erase(res_idx);

			printf("INFO: Deleting resource: %d\n", res_idx);
			fflush(stdout);
		}

		template<typename _t, typename... _args_t>
		vren::rc<_t> make_rc(_args_t&&... args)
		{
			int res_idx = m_next_res_idx++;

			m_control_block_by_res_idx.emplace(
				res_idx,
				std::unique_ptr<_ref_counted_base, _ref_counted_deleter>(
					new _ref_counted<_t>(std::forward<_args_t>(args)...),
					_ref_counted_deleter::new_<_t>()
				)
			);

			return vren::rc<_t>(this, res_idx, m_is_alive);
		}

		inline size_t get_resources_count()
		{
			return m_control_block_by_res_idx.size();
		}
	};
}
