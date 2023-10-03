#include "Pipeline.hpp"

#include "Context.hpp"

using namespace vren;

Pipeline::~Pipeline()
{
    // TODO Descriptor sets could be in-use when the pipeline is deleted, don't destroy them here (e.g. use ref count)
    for (VkDescriptorSetLayout descriptor_set_layout : m_descriptor_set_layouts)
        vkDestroyDescriptorSetLayout(Context::get().device().handle(), descriptor_set_layout, nullptr);
}

void Pipeline::bind(VkCommandBuffer command_buffer) const
{
    vkCmdBindPipeline(command_buffer, m_bind_point, m_pipeline.get());
}

void Pipeline::bind_vertex_buffer(VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset) const
{
    vkCmdBindVertexBuffers(command_buffer, binding, 1, &vertex_buffer, &offset);
}

void Pipeline::bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset) const
{
    vkCmdBindIndexBuffer(command_buffer, index_buffer, offset, index_type);
}

void Pipeline::bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set) const
{
    vkCmdBindDescriptorSets(command_buffer, m_bind_point, m_pipeline_layout.get(), descriptor_set_idx, 1, &descriptor_set, 0, nullptr);
}

void Pipeline::push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset) const
{
    vkCmdPushConstants(command_buffer, m_pipeline_layout.get(), shader_stage, offset, length, data);
}

void Pipeline::acquire_and_bind_descriptor_set(
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    uint32_t descriptor_set_idx,
    std::function<void(VkDescriptorSet)> const& update_func
)
{
    std::shared_ptr<PooledDescriptorSet> desc_set =
        std::make_shared<PooledDescriptorSet>(Context::get().toolbox()->m_descriptor_pool.acquire(m_descriptor_set_layouts.at(descriptor_set_idx)));
    update_func(desc_set->m_handle.m_descriptor_set);
    bind_descriptor_set(command_buffer, descriptor_set_idx, desc_set->m_handle.m_descriptor_set);
    resource_container.add_resources(desc_set);
}

void Pipeline::dispatch(VkCommandBuffer command_buffer, uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const
{
    vkCmdDispatch(command_buffer, workgroup_count_x, workgroup_count_y, workgroup_count_z);
}
