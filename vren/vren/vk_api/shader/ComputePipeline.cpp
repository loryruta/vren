#include "ComputePipeline.hpp"

#include <algorithm>

#include "Context.hpp"

using namespace vren;

ComputePipeline::ComputePipeline(std::shared_ptr<Shader> shader) :
    Pipeline(VK_PIPELINE_BIND_POINT_COMPUTE)
{
    m_shaders = {std::move(shader)};

    recreate();
}

void ComputePipeline::dispatch(
    uint32_t num_workgroups_x,
    uint32_t num_workgroups_y,
    uint32_t num_workgroups_z,
    VkCommandBuffer command_buffer,
    ResourceContainer& resource_container
) const
{
    vkCmdDispatch(command_buffer, num_workgroups_x, num_workgroups_y, num_workgroups_z);
    add_resources(resource_container);
}

void ComputePipeline::recreate()
{
    clear_vk_objects();

    Shader& shader = *m_shaders.at(0); // We only have one shader for the compute pipeline

    // Shader stage
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_info{};
    pipeline_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_shader_stage_info.stage = (VkShaderStageFlagBits) shader.shader_stage();
    pipeline_shader_stage_info.module = shader.handle();
    pipeline_shader_stage_info.pName = "main";

    // Push constant range
    VkPushConstantRange push_constant_range{};
    if (shader.has_push_constant_block())
    {
        push_constant_range.stageFlags = shader.shader_stage();
        push_constant_range.offset = 0;
        push_constant_range.size = (uint32_t) shader.push_constant_block_size();
    }

    // Descriptor set layouts
    m_descriptor_set_layouts.resize(shader.max_descriptor_set_id() + 1);
    for (int set_id = 0; set_id <= shader.max_descriptor_set_id(); set_id++)
    {
        DescriptorSetLayoutInfo layout_info{};
        if (shader.has_descriptor_set(set_id))
            layout_info = shader.descriptor_set_layout_info(set_id);
        m_descriptor_set_layouts.emplace_back(std::make_shared<vk_descriptor_set_layout>(layout_info.create()));
    }

    std::vector<VkDescriptorSetLayout> raw_descriptor_set_layouts{};
    std::ranges::transform(
        m_descriptor_set_layouts, std::back_inserter(raw_descriptor_set_layouts),
        [](std::shared_ptr<vk_descriptor_set_layout> const& layout) { return layout->get(); }
    );

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = (uint32_t) raw_descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = raw_descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = shader.has_push_constant_block() ? 1 : 0;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    VkPipelineLayout pipeline_layout;
    VREN_CHECK(vkCreatePipelineLayout(Context::get().device().handle(), &pipeline_layout_info, nullptr, &pipeline_layout));
    m_pipeline_layout = std::make_shared<vk_pipeline_layout>(pipeline_layout);

    // Compute pipeline
    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage = pipeline_shader_stage_info;
    pipeline_info.layout = m_pipeline_layout->get();

    VkPipeline pipeline;
    VREN_CHECK(vkCreateComputePipelines(Context::get().device().handle(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));
    m_pipeline = std::make_shared<vk_pipeline>(pipeline);
}
