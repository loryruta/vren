#pragma once

namespace vren
{
	class resource_container
	{
	public:
		std::vector<std::shared_ptr<void>> m_resources;

		explicit resource_container() = default;

		template<typename _t>
		void add_resource(std::shared_ptr<_t> const& resource)
		{
			m_resources.push_back(std::shared_ptr<_t>(resource, nullptr));
		}

		template<typename... _t>
		void add_resources(std::shared_ptr<_t> const&... resources)
		{
			(add_resource<_t>(resources), ...);
		}

		void clear()
		{
			m_resources.clear();
		}
	};
}

