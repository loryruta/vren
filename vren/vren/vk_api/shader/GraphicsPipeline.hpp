#pragma once

#include <optional>

#include <volk.h>

#include "Pipeline.hpp"

namespace vren
{
    // Forward decl
    class GraphicsPipelineBuilder;

    // ------------------------------------------------------------------------------------------------ GraphicsPipeline

    class GraphicsPipeline final : public Pipeline
    {
        friend class GraphicsPipelineBuilder;

    private:
        std::optional<VkPipelineVertexInputStateCreateInfo> m_vertex_input_state_create_info{};
        std::optional<VkPipelineInputAssemblyStateCreateInfo> m_input_assembly_state_create_info{};
        std::optional<VkPipelineTessellationStateCreateInfo> m_tessellation_state_create_info{};
        std::optional<VkPipelineViewportStateCreateInfo> m_viewport_state_create_info{};
        std::optional<VkPipelineRasterizationStateCreateInfo> m_rasterization_state_create_info{};
        std::optional<VkPipelineMultisampleStateCreateInfo> m_multisample_state_create_info{};
        std::optional<VkPipelineDepthStencilStateCreateInfo> m_depth_stencil_state_create_info{};
        std::optional<VkPipelineColorBlendStateCreateInfo> m_color_blend_state_create_info{};
        std::optional<VkPipelineDynamicStateCreateInfo> m_dynamic_state_create_info{};
        std::optional<VkPipelineRenderingCreateInfo> m_rendering_create_info{};

        explicit GraphicsPipeline();

    public:
        ~GraphicsPipeline() = default;

        void recreate() override;
    };

    // ------------------------------------------------------------------------------------------------ GraphicsPipelineBuilder

    class GraphicsPipelineBuilder
    {
    private:
        GraphicsPipeline m_graphics_pipeline;
        bool m_built = false;

    public:
        explicit GraphicsPipelineBuilder() = default;
        ~GraphicsPipelineBuilder() = default;

        GraphicsPipelineBuilder& set_shaders(std::span<std::shared_ptr<Shader>> shaders);

        GraphicsPipelineBuilder& set_vertex_input_state_create_info(VkPipelineVertexInputStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_input_assembly_state_create_info(VkPipelineInputAssemblyStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_tessellation_state_create_info(VkPipelineTessellationStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_viewport_state_create_info(VkPipelineViewportStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_rasterization_state_create_info(VkPipelineRasterizationStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_multisample_state_create_info(VkPipelineMultisampleStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_depth_stencil_state_create_info(VkPipelineDepthStencilStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_color_blend_state_create_info(VkPipelineColorBlendStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_dynamic_state_create_info(VkPipelineDynamicStateCreateInfo const& create_info);
        GraphicsPipelineBuilder& set_rendering_create_info(VkPipelineRenderingCreateInfo const& create_info);

        GraphicsPipeline&& build();
    };
}
