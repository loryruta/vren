#include "basic_renderer.hpp"

#include <array>

#include "Context.hpp"
#include "vk_helpers/debug_utils.hpp"
#include "vk_helpers/misc.hpp"

vren::basic_renderer::basic_renderer(vren::context& context) :
    m_context(context),
    m_pipeline(create_graphics_pipeline())
{
    vren::vk_utils::set_name(m_context, m_pipeline, "vertex_pipeline");
}

vren::basic_renderer::~basic_renderer() {}

vren::pipeline vren::basic_renderer::create_graphics_pipeline()
{
    // Vertex input state
    VkVertexInputBindingDescription vertex_input_binding_descriptions[]{
        {.binding = 0, .stride = sizeof(vren::vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX}, // Vertex buffer
        {.binding = 1,
         .stride = sizeof(vren::mesh_instance),
         .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE}, // Instance buffer
    };

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions[]{
        // Vertex buffer
        {.location = 0,
         .binding = 0,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = (uint32_t) offsetof(vren::vertex, m_position)}, // Position
        {.location = 1,
         .binding = 0,
         .format = VK_FORMAT_R32G32B32_SFLOAT,
         .offset = (uint32_t) offsetof(vren::vertex, m_normal)}, // Normal
        {.location = 2,
         .binding = 0,
         .format = VK_FORMAT_R32G32_SFLOAT,
         .offset = (uint32_t) offsetof(vren::vertex, m_texcoords)}, // Texcoords

        // Instance buffer
        {.location = 16, .binding = 1, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = (uint32_t) 0}, // Transform 0
        {.location = 17,
         .binding = 1,
         .format = VK_FORMAT_R32G32B32A32_SFLOAT,
         .offset = (uint32_t) sizeof(glm::vec4)}, // Transform 1
        {.location = 18,
         .binding = 1,
         .format = VK_FORMAT_R32G32B32A32_SFLOAT,
         .offset = (uint32_t) sizeof(glm::vec4) * 2}, // Transform 2
        {.location = 19,
         .binding = 1,
         .format = VK_FORMAT_R32G32B32A32_SFLOAT,
         .offset = (uint32_t) sizeof(glm::vec4) * 3}, // Transform 3
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .vertexBindingDescriptionCount = std::size(vertex_input_binding_descriptions),
        .pVertexBindingDescriptions = vertex_input_binding_descriptions,
        .vertexAttributeDescriptionCount = std::size(vertex_input_attribute_descriptions),
        .pVertexAttributeDescriptions = vertex_input_attribute_descriptions};

    /* Input assembly state */
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false};

    /* Tessellation state */

    /* Viewport state */
    VkPipelineViewportStateCreateInfo viewport_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr};

    /* Rasterization state */
    VkPipelineRasterizationStateCreateInfo rasterization_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f};

    /* Multisample state */
    VkPipelineMultisampleStateCreateInfo multisample_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE};

    /* Depth-stencil state */
    VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = true,
        .depthWriteEnable = true,
        .depthCompareOp = VK_COMPARE_OP_LESS};

    /* Color blend state */
    VkPipelineColorBlendAttachmentState color_blend_attachments[]{
        {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        },
        {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        },
        {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
        },
    };
    VkPipelineColorBlendStateCreateInfo color_blend_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .logicOpEnable = VK_FALSE,
        .logicOp = {},
        .attachmentCount = std::size(color_blend_attachments),
        .pAttachments = color_blend_attachments,
        .blendConstants = {}};

    /* Dynamic state */
    VkDynamicState dynamic_states[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .dynamicStateCount = std::size(dynamic_states),
        .pDynamicStates = dynamic_states};

    // Pipeline rendering
    VkFormat color_attachment_formats[]{
        VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R16_UINT,
    };
    VkPipelineRenderingCreateInfoKHR pipeline_rendering_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext = nullptr,
        .viewMask = 0,
        .colorAttachmentCount = std::size(color_attachment_formats),
        .pColorAttachmentFormats = color_attachment_formats,
        .depthAttachmentFormat = VREN_DEPTH_BUFFER_OUTPUT_FORMAT,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED};

    char const* vertex_shader_path = ".vren/resources/shaders/basic_draw.vert.spv";
    vren::shader_module vertex_shader_module = vren::load_shader_module_from_file(m_context, vertex_shader_path);
    vren::specialized_shader vertex_shader = vren::specialized_shader(vertex_shader_module, "main");

    vren::vk_utils::set_name(m_context, vertex_shader_module, vertex_shader_path);

    char const* fragment_shader_path = ".vren/resources/shaders/deferred.frag.spv";
    vren::shader_module fragment_shader_module = vren::load_shader_module_from_file(m_context, fragment_shader_path);
    vren::specialized_shader fragment_shader = vren::specialized_shader(fragment_shader_module, "main");

    vren::vk_utils::set_name(m_context, fragment_shader_module, fragment_shader_path);

    vren::specialized_shader shaders[] = {std::move(vertex_shader), std::move(fragment_shader)};
    return vren::create_graphics_pipeline(
        m_context,
        shaders,
        &vertex_input_state_info,
        &input_assembly_state_info,
        nullptr,
        &viewport_info,
        &rasterization_info,
        &multisample_info,
        &depth_stencil_info,
        &color_blend_info,
        &dynamic_state_info,
        &pipeline_rendering_info,
        VK_NULL_HANDLE,
        0
    );
}

void vren::basic_renderer::make_rendering_scope(
    VkCommandBuffer cmd_buf,
    glm::uvec2 const& screen,
    vren::gbuffer const& gbuffer,
    vren::vk_utils::depth_buffer_t const& depth_buffer,
    std::function<void()> const& callback
)
{
    // GBuffer
    VkRenderingAttachmentInfoKHR color_attachments[]{
        {
            // Normal buffer
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .pNext = nullptr,
            .imageView = gbuffer.m_normal_buffer.get_image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        },
        {
            // Texcoord buffer
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .pNext = nullptr,
            .imageView = gbuffer.m_texcoord_buffer.get_image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        },
        {
            // Material index buffer
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
            .pNext = nullptr,
            .imageView = gbuffer.m_material_index_buffer.get_image_view(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        },
    };

    // Depth buffer
    VkRenderingAttachmentInfoKHR depth_buffer_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .pNext = nullptr,
        .imageView = depth_buffer.get_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

    // Begin rendering
    VkRenderingInfoKHR rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .pNext = nullptr,
        .flags = NULL,
        .renderArea = {.offset = {0, 0}, .extent = {screen.x, screen.y}},
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = std::size(color_attachments),
        .pColorAttachments = color_attachments,
        .pDepthAttachment = &depth_buffer_attachment,
        .pStencilAttachment = nullptr};
    vkCmdBeginRendering(cmd_buf, &rendering_info);

    // Begin pipeline
    m_pipeline.bind(cmd_buf);

    callback();

    // End rendering
    vkCmdEndRendering(cmd_buf);
}

void vren::basic_renderer::set_viewport(VkCommandBuffer cmd_buf, float width, float height)
{
    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = height;
    viewport.width = width;
    viewport.height = -height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
}

void vren::basic_renderer::set_scissor(VkCommandBuffer cmd_buf, uint32_t width, uint32_t height)
{
    VkRect2D rect2d{};
    rect2d.offset.x = 0;
    rect2d.offset.y = 0;
    rect2d.extent.width = width;
    rect2d.extent.height = height;
    vkCmdSetScissor(cmd_buf, 0, 1, &rect2d);
}

void vren::basic_renderer::set_push_constants(
    VkCommandBuffer cmd_buf, vren::basic_renderer::push_constants const& push_constants
)
{
    m_pipeline.push_constants(cmd_buf, VK_SHADER_STAGE_VERTEX_BIT, &push_constants, sizeof(push_constants), 0);
}

void vren::basic_renderer::set_vertex_buffer(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& vertex_buffer)
{
    m_pipeline.bind_vertex_buffer(cmd_buf, 0, vertex_buffer.m_buffer.m_handle);
}

void vren::basic_renderer::set_index_buffer(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& index_buffer)
{
    m_pipeline.bind_index_buffer(cmd_buf, index_buffer.m_buffer.m_handle, VK_INDEX_TYPE_UINT32);
}

void vren::basic_renderer::set_instance_buffer(VkCommandBuffer cmd_buf, vren::vk_utils::buffer const& instance_buffer)
{
    m_pipeline.bind_vertex_buffer(cmd_buf, 1, instance_buffer.m_buffer.m_handle);
}

vren::render_graph_t vren::basic_renderer::render(
    vren::render_graph_allocator& render_graph_allocator,
    glm::uvec2 const& screen,
    vren::Camera const& camera,
    vren::basic_model_draw_buffer const& draw_buffer,
    vren::gbuffer const& gbuffer,
    vren::vk_utils::depth_buffer_t const& depth_buffer
)
{
    vren::render_graph_node* node = render_graph_allocator.allocate();

    node->set_name("basic_renderer");

    node->set_src_stage(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    node->set_dst_stage(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    gbuffer.add_render_graph_node_resources(
        *node, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    );
    node->add_image(
        {
            .m_image = depth_buffer.get_image(),
            .m_image_aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        },
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    );

    node->set_callback(
        [this, screen, camera, &draw_buffer, &gbuffer, &depth_buffer](
            uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container
        )
        {
            VkRect2D render_area = {.offset = {0, 0}, .extent = {screen.x, screen.y}};

            // GBuffer
            VkRenderingAttachmentInfoKHR color_attachments[]{
                {
                    // Normal buffer
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .pNext = nullptr,
                    .imageView = gbuffer.m_normal_buffer.get_image_view(),
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                },
                {
                    // Texcoord buffer
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .pNext = nullptr,
                    .imageView = gbuffer.m_texcoord_buffer.get_image_view(),
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                },
                {
                    // Material index buffer
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .pNext = nullptr,
                    .imageView = gbuffer.m_material_index_buffer.get_image_view(),
                    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .resolveMode = VK_RESOLVE_MODE_NONE,
                    .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                },
            };

            // Depth buffer
            VkRenderingAttachmentInfoKHR depth_buffer_attachment{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .pNext = nullptr,
                .imageView = depth_buffer.get_image_view(),
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE};

            // Begin rendering
            VkRenderingInfoKHR rendering_info{
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .pNext = nullptr,
                .flags = NULL,
                .renderArea = render_area,
                .layerCount = 1,
                .viewMask = 0,
                .colorAttachmentCount = std::size(color_attachments),
                .pColorAttachments = color_attachments,
                .pDepthAttachment = &depth_buffer_attachment,
                .pStencilAttachment = nullptr};
            vkCmdBeginRendering(command_buffer, &rendering_info);

            m_pipeline.bind(command_buffer);

            VkViewport viewport{
                .x = 0,
                .y = (float) screen.y,
                .width = (float) screen.x,
                .height = -((float) screen.y),
                .minDepth = 0.0f,
                .maxDepth = 1.0f};
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);
            vkCmdSetScissor(command_buffer, 0, 1, &render_area);

            struct
            {
                glm::mat4 m_camera_view;
                glm::mat4 m_camera_projection;
                glm::uint m_material_index;
                float _pad[3];
            } push_constants;

            push_constants = {
                .m_camera_view = camera.get_view(),
                .m_camera_projection = camera.get_projection(),
            };

            m_pipeline.bind_vertex_buffer(
                command_buffer, 0, draw_buffer.m_vertex_buffer.m_buffer.m_handle
            ); // Vertex buffer
            m_pipeline.bind_vertex_buffer(
                command_buffer, 1, draw_buffer.m_instance_buffer.m_buffer.m_handle
            ); // Instance buffer
            m_pipeline.bind_index_buffer(
                command_buffer, draw_buffer.m_index_buffer.m_buffer.m_handle, VK_INDEX_TYPE_UINT32
            ); // Index buffer

            for (vren::basic_model_draw_buffer::mesh const& mesh : draw_buffer.m_meshes)
            {
                push_constants.m_material_index = mesh.m_material_idx;

                m_pipeline.push_constants(
                    command_buffer, VK_SHADER_STAGE_VERTEX_BIT, &push_constants, sizeof(push_constants), 0
                );

                vkCmdDrawIndexed(
                    command_buffer,
                    mesh.m_index_count,
                    mesh.m_instance_count,
                    mesh.m_index_offset,
                    mesh.m_vertex_offset,
                    mesh.m_instance_offset
                );
            }

            // End rendering
            vkCmdEndRendering(command_buffer);
        }
    );
    return vren::render_graph_gather(node);
}
