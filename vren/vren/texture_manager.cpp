#include "texture_manager.hpp"

#include "context.hpp"
#include "toolbox.hpp"
#include "vk_helpers/debug_utils.hpp"
#include "vk_helpers/misc.hpp"

// --------------------------------------------------------------------------------------------------------------------------------
// Texture manager descriptor pool
// --------------------------------------------------------------------------------------------------------------------------------

VkDescriptorSet vren::texture_manager_descriptor_pool::allocate_descriptor_set(
    VkDescriptorPool descriptor_pool, VkDescriptorSetLayout descriptor_set_layout
)
{
    uint32_t descriptor_count = vren::texture_manager::k_max_texture_count;
    VkDescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &descriptor_count};
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variable_descriptor_count_alloc_info,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout};
    VkDescriptorSet descriptor_set;
    VREN_CHECK(vkAllocateDescriptorSets(m_context->m_device, &descriptor_set_alloc_info, &descriptor_set), m_context);
    return descriptor_set;
}

vren::texture_manager_descriptor_pool::texture_manager_descriptor_pool(
    vren::context const& context, uint32_t max_sets, std::span<VkDescriptorPoolSize> const& pool_sizes
) :
    vren::descriptor_pool(context, max_sets, pool_sizes)
{
}

// --------------------------------------------------------------------------------------------------------------------------------
// Texture manager
// --------------------------------------------------------------------------------------------------------------------------------

vren::texture_manager::texture_manager(vren::context const& context) :
    m_context(&context),
    m_descriptor_set_layout(create_descriptor_set_layout()),
    m_descriptor_pool(create_descriptor_pool())
{
}

vren::vk_descriptor_set_layout vren::texture_manager::create_descriptor_set_layout()
{
    /* Bindings */
    VkDescriptorSetLayoutBinding bindings[]{
        {.binding = 0,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .descriptorCount = 65536, // TODO (VERY IMPORTANT) k_max_variable_count_descriptor_count used both here and by
                                   // the shader parser
         .stageFlags = VK_SHADER_STAGE_ALL,
         .pImmutableSamplers = nullptr}};

    /* Descriptor set layout */
    VkDescriptorBindingFlags bindless_flags =
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .pNext = nullptr,
        .bindingCount = 1,
        .pBindingFlags = &bindless_flags};

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &binding_flags_info,
        .flags = NULL,
        .bindingCount = std::size(bindings),
        .pBindings = bindings};
    VkDescriptorSetLayout descriptor_set_layout;
    VREN_CHECK(
        vkCreateDescriptorSetLayout(m_context->m_device, &descriptor_set_layout_info, nullptr, &descriptor_set_layout),
        m_context
    );
    return vren::vk_descriptor_set_layout(*m_context, descriptor_set_layout);
}

vren::texture_manager_descriptor_pool vren::texture_manager::create_descriptor_pool()
{
    uint32_t max_sets = VREN_MAX_FRAME_IN_FLIGHT_COUNT;
    VkDescriptorPoolSize pool_sizes[]{
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, k_max_texture_count * max_sets},
    };
    return vren::texture_manager_descriptor_pool(*m_context, VREN_MAX_FRAME_IN_FLIGHT_COUNT, pool_sizes);
}

void vren::texture_manager::rewrite_descriptor_set()
{
    m_descriptor_set =
        std::make_unique<vren::pooled_vk_descriptor_set>(m_descriptor_pool.acquire(m_descriptor_set_layout.m_handle));
    vren::vk_utils::set_object_name(
        *m_context,
        VK_OBJECT_TYPE_DESCRIPTOR_SET,
        (uint64_t) m_descriptor_set->m_handle.m_descriptor_set,
        "texture_manager"
    );

    std::vector<VkDescriptorImageInfo> image_info(m_textures.size());
    for (uint32_t i = 0; i < m_textures.size(); i++)
    {
        auto const& texture = m_textures.at(i);
        image_info[i] = {
            .sampler = texture.m_sampler.m_handle,
            .imageView = texture.m_image_view.m_handle,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    VkWriteDescriptorSet descriptor_set_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptor_set->m_handle.m_descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = (uint32_t) m_textures.size(),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = image_info.data(),
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr};
    vkUpdateDescriptorSets(m_context->m_device, 1, &descriptor_set_write, 0, nullptr);
}
