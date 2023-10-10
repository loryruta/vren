#include "DescriptorSetLayoutInfo.hpp"

#include "Context.hpp"

using namespace vren;

DescriptorSetLayoutInfo& DescriptorSetLayoutInfo::operator+=(DescriptorSetLayoutInfo const& other)
{
    m_bindings.insert(other.m_bindings.begin(), other.m_bindings.end());

    return *this;
}

VkDescriptorSetLayout DescriptorSetLayoutInfo::create() const
{
    std::vector<VkDescriptorSetLayoutBinding> vk_binding_infos{};
    vk_binding_infos.reserve(m_bindings.size());
    for (Binding const& binding_info : m_bindings)
    {
        VkDescriptorSetLayoutBinding vk_binding_info{};
        vk_binding_info.binding = binding_info.m_binding;
        vk_binding_info.descriptorType = binding_info.m_descriptor_type;
        vk_binding_info.descriptorCount = binding_info.m_descriptor_count;
        vk_binding_infos.push_back(vk_binding_info);
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pBindings = vk_binding_infos.data();
    descriptor_set_layout_info.bindingCount = (uint32_t) m_bindings.size();

    VkDescriptorSetLayout descriptor_set_layout;
    VREN_CHECK(vkCreateDescriptorSetLayout(Context::get().device().handle(), &descriptor_set_layout_info, nullptr, &descriptor_set_layout));
    return descriptor_set_layout;
}
