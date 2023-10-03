#pragma once

#include <vector>

#include <volk.h>

#include "wrappers/HandleDeleter.hpp"

namespace vren
{
    class Pipeline
    {
    private:
        std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
        HandleDeleter<VkPipelineLayout> m_pipeline_layout;
        HandleDeleter<VkPipeline> m_pipeline;

        VkPipelineBindPoint m_bind_point;

    public:
        ~Pipeline();

        void bind(VkCommandBuffer command_buffer) const;
        void bind_vertex_buffer(VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset = 0) const;
        void bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset = 0) const;
        void bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set) const;
        void push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset = 0) const;

        void acquire_and_bind_descriptor_set(
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            uint32_t descriptor_set_idx,
            std::function<void(VkDescriptorSet descriptor_set)> const& update_func
        );

        void dispatch(VkCommandBuffer command_buffer, uint32_t workgroup_count_x, uint32_t workgroup_count_y, uint32_t workgroup_count_z) const;
    };
} // namespace vren