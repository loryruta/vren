#include "Pipeline.hpp"

#include "Context.hpp"

using namespace vren;

Pipeline::Pipeline(VkPipelineBindPoint bind_point) :
    m_bind_point(bind_point)
{
}

void Pipeline::bind(VkCommandBuffer command_buffer) const
{
    vkCmdBindPipeline(command_buffer, m_bind_point, m_pipeline->get());
}

void Pipeline::push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset) const
{
    vkCmdPushConstants(command_buffer, m_pipeline_layout->get(), shader_stage, offset, length, data);
}

void Pipeline::bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t set_id, VkDescriptorSet set) const
{
    vkCmdBindDescriptorSets(command_buffer, m_bind_point, m_pipeline_layout->get(), set_id, 1, &set, 0, nullptr);
}

void Pipeline::acquire_and_bind_descriptor_set(
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container,
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

void Pipeline::add_resources(ResourceContainer& resource_container) const
{
    resource_container.add_resource(m_pipeline);
    resource_container.add_resource(m_pipeline_layout);
    resource_container.add_span(m_descriptor_set_layouts);
}
