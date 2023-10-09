#include "Pipeline.hpp"

#include "Context.hpp"

using namespace vren;

Pipeline::Pipeline(VkPipelineBindPoint bind_point, std::span<std::shared_ptr<Shader>> shaders) :
    m_bind_point(bind_point),
    m_shaders(shaders.begin(), shaders.end())
{
    assert(!m_shaders.empty());
}

Pipeline::~Pipeline()
{
    // TODO Descriptor sets could be in-use when the pipeline is deleted, don't destroy them here (e.g. use ref count)
    for (VkDescriptorSetLayout descriptor_set_layout : m_descriptor_set_layouts)
        vkDestroyDescriptorSetLayout(Context::get().device().handle(), descriptor_set_layout, nullptr);
}

void Pipeline::bind(VkCommandBuffer command_buffer) const
{
    vkCmdBindPipeline(command_buffer, m_bind_point, m_handle->get());
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
    vkCmdBindDescriptorSets(command_buffer, m_bind_point, m_pipeline_layout->get(), descriptor_set_idx, 1, &descriptor_set, 0, nullptr);
}

void Pipeline::push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset) const
{
    vkCmdPushConstants(command_buffer, m_pipeline_layout->get(), shader_stage, offset, length, data);
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

void Pipeline::dispatch(VkCommandBuffer command_buffer, uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const
{
}

void Pipeline::recreate()
{
    m_pipeline_layout.reset();
    m_pipeline.reset();

    // Push constants
    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = m_shader->m_shader_stage;
    push_constant_range.offset = 0;
    push_constant_range.size = (uint32_t) m_shader->m_push_constant_block_size;

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = (uint32_t) descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = shader_module.has_push_constant_block() ? 1u : 0u;
    pipeline_layout_info.pPushConstantRanges = shader_module.has_push_constant_block() ? &push_constant_range : nullptr;

    VkPipelineLayout pipeline_layout_handle;
    VREN_CHECK(vkCreatePipelineLayout(Context::get().device().handle(), &pipeline_layout_info, nullptr, &pipeline_layout_handle));
    m_pipeline_layout = std::make_shared<HandleDeleter<VkPipelineLayout>>(pipeline_layout_handle);

    // Shader stage
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages_info;
    shader_stages_info.reserve(m_shaders.size());
    for (std::shared_ptr<Shader> const& shader : m_shaders)
    {
        VkPipelineShaderStageCreateInfo shader_stage_info{};
        shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info.stage = (VkShaderStageFlagBits) shader->shader_stage();
        shader_stage_info.module = shader->m_handle->get();
        shader_stage_info.pName = "main";
        shader_stages_info.push_back(shader_stage_info);
    }

    // Compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = pipeline_shader_stage_info;
    pipeline_info.layout = m_pipeline_layout->get();

    VkPipeline pipeline_handle;
    VREN_CHECK(vkCreateComputePipelines(Context::get().device().handle(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_handle));
    m_pipeline = std::make_shared<HandleDeleter<VkPipeline>>(pipeline_handle);
}

void Pipeline::add_command_buffer_resources(vren::ResourceContainer& resource_container)
{
    resource_container.add_resource(m_pipeline_layout);
    resource_container.add_resource(m_handle);
}
