#include "build_bvh.hpp"

#include <glm/gtc/integer.hpp>

#include "toolbox.hpp"
#include "vk_helpers/misc.hpp"

vren::build_bvh::build_bvh(vren::context const& context) :
    m_context(&context),
    m_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(context, ".vren/resources/shaders/build_bvh.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module);
        return vren::create_compute_pipeline(context, shader);
    }())
{
}

void vren::build_bvh::write_descriptor_set(
    VkDescriptorSet descriptor_set,
    vren::vk_utils::buffer const& buffer,
    uint32_t leaf_count
)
{
    vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 0, buffer.m_buffer.m_handle, get_required_buffer_size(leaf_count), 0);
}

VkBufferUsageFlags vren::build_bvh::get_required_buffer_usage_flags()
{
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

size_t vren::build_bvh::get_required_buffer_size(uint32_t leaf_count)
{
    return calc_bvh_buffer_size(leaf_count);
}

void vren::build_bvh::operator()(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& buffer,
    uint32_t leaf_count,
    uint32_t* root_node_idx
)
{
    assert(vren::is_power_of(leaf_count, 32u));

    uint32_t level_count = leaf_count;

    uint32_t length = calc_bvh_buffer_length(leaf_count);

    if (root_node_idx != nullptr)
    {
        *root_node_idx = length - 1;
    }

    m_pipeline.bind(command_buffer);

    auto descriptor_set = std::make_shared<vren::pooled_vk_descriptor_set>(
        m_context->m_toolbox->m_descriptor_pool.acquire(m_pipeline.m_descriptor_set_layouts.at(0))
    );
    resource_container.add_resource(descriptor_set);
    write_descriptor_set(descriptor_set->m_handle.m_descriptor_set, buffer, leaf_count);

    m_pipeline.bind_descriptor_set(command_buffer, 0, descriptor_set->m_handle.m_descriptor_set);

    struct {
        uint32_t m_src_level_idx;
        uint32_t m_dst_level_idx;
    } push_constants;

    push_constants = {};

    while (level_count > 1)
    {
        push_constants.m_src_level_idx = push_constants.m_dst_level_idx;
        push_constants.m_dst_level_idx = push_constants.m_src_level_idx + level_count;

        m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &push_constants, sizeof(push_constants));

        uint32_t num_workgroups = vren::divide_and_ceil(level_count, k_workgroup_size);
        m_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);

        level_count >>= 5;

        if (level_count > 1)
        {
            VkBufferMemoryBarrier buffer_memory_barrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .pNext = nullptr,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .buffer = buffer.m_buffer.m_handle,
                .offset = push_constants.m_dst_level_idx * sizeof(vren::bvh_node),
                .size = level_count * sizeof(vren::bvh_node)
            };
            vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);
        }
    }
}

uint32_t vren::calc_bvh_padded_leaf_count(uint32_t leaf_count)
{
    return vren::round_to_next_power_of(leaf_count, 32u);
}

uint32_t vren::calc_bvh_buffer_length(uint32_t leaf_count)
{
    uint32_t padded_leaf_count = calc_bvh_padded_leaf_count(leaf_count);
    size_t length = 0;
    while (padded_leaf_count != 0)
    {
        assert(padded_leaf_count % 32 == 0 || padded_leaf_count == 1);

        length += padded_leaf_count;
        padded_leaf_count >>= 5;
    }
    return length;
}

size_t vren::calc_bvh_buffer_size(uint32_t leaf_count)
{
    uint32_t padded_leaf_count = calc_bvh_padded_leaf_count(leaf_count);
    return calc_bvh_buffer_length(padded_leaf_count) * sizeof(vren::bvh_node);
}

uint32_t vren::calc_bvh_root_index(uint32_t leaf_count)
{
    uint32_t padded_leaf_count = calc_bvh_padded_leaf_count(leaf_count);
    return vren::calc_bvh_buffer_length(padded_leaf_count) - 1;
}

uint32_t vren::calc_bvh_level_count(uint32_t leaf_count)
{
    uint32_t padded_leaf_count = calc_bvh_padded_leaf_count(leaf_count);
    return (glm::log2(padded_leaf_count) / 5) + 1;
}
