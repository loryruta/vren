#include "DescriptorSetLayoutCache.hpp"

using namespace vren;

std::shared_ptr<HandleDeleter<VkDescriptorSetLayout>>
DescriptorSetLayoutCache::get_or_create_descriptor_set_layout(DescriptorSetLayoutInfo const& descriptor_set_layout_info)
{
    if (m_cache.contains(descriptor_set_layout_info))
        return m_cache.at(descriptor_set_layout_info);

    VkDescriptorSetLayout descriptor_set_layout_handle = descriptor_set_layout_info.create_descriptor_set_layout();
    std::shared_ptr<HandleDeleter<VkDescriptorSetLayout>> descriptor_set_layout =
        std::make_shared<HandleDeleter<VkDescriptorSetLayout>>(descriptor_set_layout_handle);
    m_cache.emplace(descriptor_set_layout_info, descriptor_set_layout);
    return descriptor_set_layout;
}
