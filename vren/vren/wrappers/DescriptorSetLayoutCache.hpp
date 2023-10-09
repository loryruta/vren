#pragma once

#include <memory>
#include <unordered_map>

#include <volk.h>

#include "DescriptorSetLayoutInfo.hpp"
#include "HandleDeleter.hpp"

namespace vren
{
    class DescriptorSetLayoutCache
    {
    private:
        std::unordered_map<DescriptorSetLayoutInfo, std::shared_ptr<HandleDeleter<VkDescriptorSetLayout>>> m_cache;

    public:
        explicit DescriptorSetLayoutCache() = default;
        ~DescriptorSetLayoutCache() = default;

        std::shared_ptr<HandleDeleter<VkDescriptorSetLayout>> get_or_create_descriptor_set_layout(DescriptorSetLayoutInfo const& descriptor_set_layout_info);
    };
} // namespace vren
