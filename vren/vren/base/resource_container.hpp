#pragma once

#include <memory>
#include <vector>

namespace vren
{
    class resource_container
    {
    public:
        std::vector<std::shared_ptr<void>> m_resources;

        explicit resource_container() = default;

        template <typename _t> void add_resource(std::shared_ptr<_t> const& resource)
        {
            m_resources.push_back(std::shared_ptr<_t>(resource, nullptr));
        }

        template <typename... _t> void add_resources(std::shared_ptr<_t> const&... resources)
        {
            (add_resource<_t>(resources), ...);
        }

        void inherit(vren::resource_container const& other)
        {
            for (std::shared_ptr<void> const& resource : other.m_resources)
                add_resource(resource);
        }

        void clear() { m_resources.clear(); }
    };
} // namespace vren
