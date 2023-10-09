#include "ComputePipeline.hpp"

#include "Context.hpp"

using namespace vren;

ComputePipeline::ComputePipeline(std::shared_ptr<Shader>& shader) :
    Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE, std::span(&shader, 1)),
    m_shader(shader)
{
    recreate();
}

void ComputePipeline::dispatch(
    uint32_t workgroup_count_x,
    uint32_t workgroup_count_y,
    uint32_t workgroup_count_z,
    VkCommandBuffer command_buffer,
    ResourceContainer& resource_container
) const
{
}

void ComputePipeline::recreate()
{
    m_pipeline_layout.reset();
    m_pipeline.reset();

    // Push constants
    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = m_shader->shader_stage();
    push_constant_range.offset = 0;
    push_constant_range.size = (uint32_t) m_shader->m_push_constant_block_size;

    // Descriptor set layouts
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

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
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_info{};
    pipeline_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_info.stage = (VkShaderStageFlagBits) m_shader->shader_stage();
    pipeline_shader_stage_info.module = m_shader->m_handle->get();
    pipeline_shader_stage_info.pName = "main";

    // Compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = pipeline_shader_stage_info;
    pipeline_info.layout = m_pipeline_layout->get();

    VkPipeline pipeline_handle;
    VREN_CHECK(vkCreateComputePipelines(Context::get().device().handle(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_handle));
    m_pipeline = std::make_shared<HandleDeleter<VkPipeline>>(pipeline_handle);
}
