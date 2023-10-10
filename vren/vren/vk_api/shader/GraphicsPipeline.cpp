#include "GraphicsPipeline.hpp"

#include "Context.hpp"

#include <cassert>

using namespace vren;

// ------------------------------------------------------------------------------------------------ GraphicsPipeline

GraphicsPipeline::GraphicsPipeline() :
    Pipeline(VK_PIPELINE_BIND_POINT_GRAPHICS)
{
}

void GraphicsPipeline::recreate()
{
    clear_vk_objects();

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
    std::vector<VkPushConstantRange> push_constant_ranges{};
    std::unordered_map<uint32_t, DescriptorSetLayoutInfo> merged_descriptor_set_layout_info_map{};
    int max_descriptor_set_idx = -1;

    for (std::shared_ptr<Shader> const& shader : m_shaders)
    {
        // Shader stage
        VkPipelineShaderStageCreateInfo shader_stage_info{};
        shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_info.stage = (VkShaderStageFlagBits) shader->shader_stage();
        shader_stage_info.module = shader->handle();
        shader_stage_info.pName = "main";
        //shader_stage_info.pSpecializationInfo = nullptr;
        shader_stages.push_back(shader_stage_info);

        // Push constant range
        if (shader->has_push_constant_block())
        {
            VkPushConstantRange push_constant_range{};
            push_constant_range.stageFlags = shader->shader_stage();
            push_constant_range.offset = 0;
            push_constant_range.size = (uint32_t) shader->push_constant_block_size();
            push_constant_ranges.push_back(push_constant_range);
        }

        // Descriptor set layout
        max_descriptor_set_idx = std::max((int) shader->max_descriptor_set_id(), max_descriptor_set_idx);

        for (int set_id = 0; set_id < shader->max_descriptor_set_id(); set_id++)
        {
            if (shader->has_descriptor_set(set_id))
            {
                DescriptorSetLayoutInfo const& layout_info = shader->descriptor_set_layout_info(set_id);
                if (merged_descriptor_set_layout_info_map.contains(set_id))
                    merged_descriptor_set_layout_info_map[set_id] += layout_info; // This could throw in case the two layout info are not compatible
                else
                {
                    merged_descriptor_set_layout_info_map[set_id] = layout_info;
                }
            }
        }
    }

    // Global descriptor set layouts
    m_descriptor_set_layouts.resize(max_descriptor_set_idx + 1);
    for (int set_idx = 0; set_idx <= max_descriptor_set_idx; set_idx++)
    {
        DescriptorSetLayoutInfo layout_info{};
        if (merged_descriptor_set_layout_info_map.contains(set_idx))
            layout_info = merged_descriptor_set_layout_info_map[set_idx];
        else
        { // Otherwise we create an empty descriptor set layout, only meant to occupy the set ID (we can't just skip it)
        }

        m_descriptor_set_layouts[set_idx] = std::make_shared<HandleDeleter<VkDescriptorSetLayout>>(layout_info.create());
    }

    std::vector<VkDescriptorSetLayout> raw_descriptor_set_layouts{};
    std::ranges::transform(
        m_descriptor_set_layouts,
        std::back_inserter(raw_descriptor_set_layouts),
        [](std::shared_ptr<HandleDeleter<VkDescriptorSetLayout>> const& layout) { return layout->get(); }
    );

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = (uint32_t) raw_descriptor_set_layouts.size();
    pipeline_layout_info.pSetLayouts = raw_descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = (uint32_t) push_constant_ranges.size();
    pipeline_layout_info.pPushConstantRanges = push_constant_ranges.data();

    VkPipelineLayout pipeline_layout;
    VREN_CHECK(vkCreatePipelineLayout(Context::get().device().handle(), &pipeline_layout_info, nullptr, &pipeline_layout));
    m_pipeline_layout = std::make_shared<HandleDeleter<VkPipelineLayout>>(pipeline_layout);

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = (uint32_t) shader_stages.size();
    pipeline_info.pStages = shader_stages.data();
    if (m_vertex_input_state_create_info) pipeline_info.pVertexInputState = &m_vertex_input_state_create_info.value();
    if (m_input_assembly_state_create_info) pipeline_info.pInputAssemblyState = &m_input_assembly_state_create_info.value();
    if (m_tessellation_state_create_info) pipeline_info.pTessellationState = &m_tessellation_state_create_info.value();
    if (m_viewport_state_create_info) pipeline_info.pViewportState = &m_viewport_state_create_info.value();
    if (m_rasterization_state_create_info) pipeline_info.pRasterizationState = &m_rasterization_state_create_info.value();
    if (m_multisample_state_create_info) pipeline_info.pMultisampleState = &m_multisample_state_create_info.value();
    if (m_depth_stencil_state_create_info) pipeline_info.pDepthStencilState = &m_depth_stencil_state_create_info.value();
    if (m_color_blend_state_create_info) pipeline_info.pColorBlendState = &m_color_blend_state_create_info.value();
    if (m_dynamic_state_create_info) pipeline_info.pDynamicState = &m_dynamic_state_create_info.value();
    if (m_rendering_create_info) pipeline_info.pNext = &m_rendering_create_info.value();
    pipeline_info.layout = pipeline_layout;
    //pipeline_info.renderPass = VK_NULL_HANDLE; // We use dynamic rendering

    VkPipeline pipeline;
    VREN_CHECK(vkCreateGraphicsPipelines(Context::get().device().handle(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));
    m_pipeline = std::make_shared<HandleDeleter<VkPipeline>>(pipeline);
}

// ------------------------------------------------------------------------------------------------ GraphicsPipelineBuilder

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_shaders(std::span<std::shared_ptr<Shader>> shaders)
{
    // TODO verify this Builder is not built already
    // TODO there shouldn't be two shaders of the same stage
    // TODO there should be at least one shader
    // TODO shaders must be compiled (this should be guaranteed because we use a builder and Shader is always compiled)

    m_graphics_pipeline.m_shaders.clear();
    m_graphics_pipeline.m_shaders.insert(m_graphics_pipeline.m_shaders.end(), shaders.begin(), shaders.end());
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_vertex_input_state_create_info(VkPipelineVertexInputStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_vertex_input_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_input_assembly_state_create_info(VkPipelineInputAssemblyStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_input_assembly_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_tessellation_state_create_info(VkPipelineTessellationStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_tessellation_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_viewport_state_create_info(VkPipelineViewportStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_viewport_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder&
GraphicsPipelineBuilder::set_rasterization_state_create_info(VkPipelineRasterizationStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_rasterization_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_multisample_state_create_info(VkPipelineMultisampleStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_multisample_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_depth_stencil_state_create_info(VkPipelineDepthStencilStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_depth_stencil_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_color_blend_state_create_info(VkPipelineColorBlendStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_color_blend_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_dynamic_state_create_info(VkPipelineDynamicStateCreateInfo const& create_info)
{
    m_graphics_pipeline.m_dynamic_state_create_info = create_info;
    return *this;
}

GraphicsPipelineBuilder& GraphicsPipelineBuilder::set_rendering_create_info(VkPipelineRenderingCreateInfo const& create_info)
{
    m_graphics_pipeline.m_rendering_create_info = create_info;
    return *this;
}

GraphicsPipeline&& GraphicsPipelineBuilder::build()
{
    assert(m_graphics_pipeline.m_shaders.size() > 0); // TODO not an assert

    m_built = true;
    return std::move(m_graphics_pipeline);
}
