#pragma once

#include <memory>
#include <vector>

namespace vren
{
    class ResourceContainer
    {
    private:
        std::vector<std::shared_ptr<void>> m_resources;

    public:
        explicit ResourceContainer() = default;
        ~ResourceContainer() = default;

        template<typename T>
        void add_resource(std::shared_ptr<T> const& resource)
        {
            m_resources.push_back(std::shared_ptr<T>(resource, nullptr));
        }

        template<typename... T>
        void add_resources(std::shared_ptr<T> const&... resources)
        {
            (add_resource<T>(resources), ...);
        }

        void inherit(vren::ResourceContainer const& other)
        {
            for (std::shared_ptr<void> const& resource : other.m_resources)
                add_resource(resource);
        }

        void clear() { m_resources.clear(); }
    };
} // namespace vren
