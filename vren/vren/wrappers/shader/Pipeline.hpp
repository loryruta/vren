#pragma once

#include <functional>
#include <memory>
#include <span>
#include <vector>
#include <unordered_map>

#include <volk.h>

#include "Shader.hpp"
#include "base/ResourceContainer.hpp"
#include "wrappers/HandleDeleter.hpp"

namespace vren
{
    class Pipeline
    {
        friend class Shader;

    protected:
        VkPipelineBindPoint m_bind_point;
        std::vector<std::shared_ptr<Shader>> m_shaders;

        std::unordered_map<DescriptorSetLayoutInfo> m_descriptor_set_layouts;

        std::shared_ptr<HandleDeleter<VkPipeline>> m_pipeline;
        std::shared_ptr<HandleDeleter<VkPipelineLayout>> m_pipeline_layout;

        explicit Pipeline(
            VkPipelineBindPoint bind_point,
            std::span<std::shared_ptr<Shader>> shaders
            );

    public:
        ~Pipeline();

        bool try_merge_descriptor_set_layout(DescriptorSetLayoutInfo const& layout_info);

        void bind(VkCommandBuffer command_buffer) const;
        void bind_vertex_buffer(VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset = 0) const;
        void bind_index_buffer(VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset = 0) const;
        void bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set) const;
        void push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset = 0) const;

        void acquire_and_bind_descriptor_set(
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container,
            uint32_t descriptor_set_idx,
            std::function<void(VkDescriptorSet descriptor_set)> const& update_func
        );

        virtual void recreate() = 0;

    private:
        void add_command_buffer_resources(ResourceContainer& resource_container);
    };
} // namespace vren