#include "Toolbox.hpp"

#include "Context.hpp"
#include "vk_helpers/misc.hpp"

using namespace vren;

Toolbox::Toolbox() :
    m_texture_manager(context),

    m_reduce_uint_add(context),
    m_reduce_uint_min(context),
    m_reduce_uint_max(context),
    m_reduce_vec4_add(context),
    m_reduce_vec4_min(context),
    m_reduce_vec4_max(context),

    m_blelloch_scan(context),
    m_radix_sort(context),
    m_bucket_sort(context),

    m_build_bvh(context)
{
    m_command_pool = std::make_unique<CommandPool>(CommandPool::create());
    init_descriptor_pool();

}

Toolbox::~Toolbox() {}

void Toolbox::init_descriptor_pool()
{
    std::vector<VkDescriptorPoolSize> pool_sizes{
        {VK_DESCRIPTOR_TYPE_SAMPLER, 32},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 32},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 32},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32}};
    m_descriptor_pool = std::make_unique<DescriptorPool>(32, pool_sizes);
}

Toolbox& Toolbox::get()
{
    return Context::get().toolbox();
}
