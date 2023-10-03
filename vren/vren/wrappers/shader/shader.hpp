#pragma once

#include <functional>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include <volk.h>
#include "base/base.hpp"
#include "base/resource_container.hpp"

namespace vren
{
    // Forward decl
    class context;

    // ------------------------------------------------------------------------------------------------

    static constexpr uint32_t k_max_variable_count_descriptor_count = 65536;

    // ------------------------------------------------------------------------------------------------
    // Shader module
    // ------------------------------------------------------------------------------------------------

    // void print_descriptor_set_layouts(std::unordered_map<uint32_t, vren::shader_module_descriptor_set_layout_info_t>
    // const& descriptor_set_layouts);

    // ------------------------------------------------------------------------------------------------
    // Specialized shader
    // ------------------------------------------------------------------------------------------------

    // ------------------------------------------------------------------------------------------------
    // Pipeline
    // ------------------------------------------------------------------------------------------------

    vren::pipeline create_compute_pipeline(vren::context const& context, vren::specialized_shader const& shader);

    pipeline create_graphics_pipeline(
        vren::context const& context,
        std::span<vren::specialized_shader const> shaders,
        VkPipelineVertexInputStateCreateInfo* vtx_input_state_info,
        VkPipelineInputAssemblyStateCreateInfo* input_assembly_state_info,
        VkPipelineTessellationStateCreateInfo* tessellation_state_info,
        VkPipelineViewportStateCreateInfo* viewport_state_info,
        VkPipelineRasterizationStateCreateInfo* rasterization_state_info,
        VkPipelineMultisampleStateCreateInfo* multisample_state_info,
        VkPipelineDepthStencilStateCreateInfo* depth_stencil_state_info,
        VkPipelineColorBlendStateCreateInfo* color_blend_state_info,
        VkPipelineDynamicStateCreateInfo* dynamic_state_info,
        VkPipelineRenderingCreateInfo* pipeline_rendering_info,
        VkRenderPass render_pass,
        uint32_t subpass
    );
} // namespace vren
