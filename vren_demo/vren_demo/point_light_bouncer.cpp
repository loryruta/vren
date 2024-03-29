#include "point_light_bouncer.hpp"

#include <vren/base/base.hpp>
#include <vren/toolbox.hpp>
#include <vren/vk_helpers/misc.hpp>

vren_demo::point_light_bouncer::point_light_bouncer(vren::context const& context) :
    m_context(&context),
    m_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "resources/shaders/bounce_point_lights.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
        return vren::create_compute_pipeline(*m_context, shader);
    }())
{}

void vren_demo::point_light_bouncer::bounce(
    uint32_t frame_idx,
    VkCommandBuffer command_buffer,
    vren::resource_container& resource_container,
    vren::vk_utils::buffer const& point_light_position_buffer,
    vren::vk_utils::buffer const& point_light_direction_buffer,
    uint32_t point_light_count,
    glm::vec3 const& aabb_min,
    glm::vec3 const& aabb_max,
    float speed,
    float dt
)
{
    m_pipeline.bind(command_buffer);

    struct push_constants
    {
        glm::vec3 m_aabb_min; float _pad;
        glm::vec3 m_aabb_max; float _pad1;
        float m_speed;
        float m_dt;
        float _pad2[2];
    };

    push_constants my_push_constants{
        .m_aabb_min = aabb_min,
        .m_aabb_max = aabb_max,
        .m_speed = speed,
        .m_dt = dt,
    };
    m_pipeline.push_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, &my_push_constants, sizeof(push_constants), 0);

    m_pipeline.acquire_and_bind_descriptor_set(*m_context, command_buffer, resource_container, 0, [&](VkDescriptorSet descriptor_set)
    {
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 0, point_light_position_buffer.m_buffer.m_handle, point_light_count * sizeof(glm::vec4), 0);
        vren::vk_utils::write_buffer_descriptor(*m_context, descriptor_set, 1, point_light_direction_buffer.m_buffer.m_handle, point_light_count * sizeof(vren::point_light), 0);
    });

    uint32_t k_num_workgroups_x = vren::divide_and_ceil(point_light_count, k_workgroup_size);
    m_pipeline.dispatch(command_buffer, k_num_workgroups_x, 1, 1);
}

vren_demo::fill_point_light_debug_draw_buffer::fill_point_light_debug_draw_buffer(vren::context const& context) :
    m_context(&context),
    m_pipeline([&]()
    {
        vren::shader_module shader_module = vren::load_shader_module_from_file(*m_context, "./resources/shaders/fill_point_light_debug_draw_buffer.comp.spv");
        vren::specialized_shader shader = vren::specialized_shader(shader_module, "main");
        return vren::create_compute_pipeline(*m_context, shader);
    }())
{
}

void vren_demo::fill_point_light_debug_draw_buffer::write_descriptor_set(
    VkDescriptorSet descriptor_set,
    vren::vk_utils::buffer const& point_light_buffer,
    uint32_t point_light_count,
    vren::debug_renderer_draw_buffer const& debug_draw_buffer
)
{
    VkDescriptorBufferInfo buffer_info;
    VkWriteDescriptorSet write_descriptor_set;

    // Point light buffer
    buffer_info = {
        .buffer = point_light_buffer.m_buffer.m_handle,
        .offset = 0,
        .range = point_light_count > 0 ? point_light_count * sizeof(vren::point_light) : 1
    };
    write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &buffer_info
    };
    vkUpdateDescriptorSets(m_context->m_device, 1, &write_descriptor_set, 0, nullptr);

    // Debug draw buffer
    buffer_info = {
        .buffer = debug_draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };
    write_descriptor_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &buffer_info
    };
    vkUpdateDescriptorSets(m_context->m_device, 1, &write_descriptor_set, 0, nullptr);
}

vren::render_graph_t vren_demo::fill_point_light_debug_draw_buffer::operator()(
    vren::render_graph_allocator& allocator,
    vren::vk_utils::buffer const& point_light_buffer,
    uint32_t point_light_count,
    vren::debug_renderer_draw_buffer const& debug_draw_buffer
)
{
    vren::render_graph_node* node = allocator.allocate();
    node->set_name("fill_point_light_debug_draw_buffer");
    node->set_src_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    node->set_dst_stage(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    node->add_buffer({
        .m_name = "point_light_buffer",
        .m_buffer = point_light_buffer.m_buffer.m_handle,
    }, VK_ACCESS_SHADER_READ_BIT);
    node->add_buffer({
        .m_name = "debug_draw_buffer",
        .m_buffer = debug_draw_buffer.m_vertex_buffer.m_buffer->m_buffer.m_handle,
    }, VK_ACCESS_SHADER_WRITE_BIT);
    node->set_callback([=, &point_light_buffer, &debug_draw_buffer](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        m_pipeline.bind(command_buffer);

        m_pipeline.acquire_and_bind_descriptor_set(*m_context, command_buffer, resource_container, 0, [&](VkDescriptorSet descriptor_set)
        {
            write_descriptor_set(descriptor_set, point_light_buffer, point_light_count, debug_draw_buffer);
        });

        uint32_t num_workgroups = vren::divide_and_ceil(point_light_count, 1024);
        m_pipeline.dispatch(command_buffer, num_workgroups, 1, 1);
    });
    return vren::render_graph_gather(node);
}
