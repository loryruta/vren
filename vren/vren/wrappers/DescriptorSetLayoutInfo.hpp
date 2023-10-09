#pragma once

#include <unordered_map>

#include <volk.h>

namespace vren
{
    struct DescriptorSetLayoutInfo
    {
        struct Binding
        {
            uint32_t m_binding;
            VkDescriptorType m_descriptor_type;
            uint32_t m_descriptor_count; // Count 0 means it's a variable descriptor count

            bool is_variable_descriptor_count() const { return m_descriptor_count == 0; }

            bool operator==(Binding const& other) const;
        };

        std::vector<Binding> m_bindings;

        VkDescriptorSetLayout create_descriptor_set_layout() const;

        DescriptorSetLayoutInfo& operator+=(DescriptorSetLayoutInfo const& other);
        bool operator==(DescriptorSetLayoutInfo const& other) const;
    };
} // namespace vren
