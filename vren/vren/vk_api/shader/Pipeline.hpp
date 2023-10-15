#pragma once

#include <functional>
#include <memory>
#include <span>
#include <vector>
#include <unordered_map>

#include <volk.h>

#include "Shader.hpp"
#include "base/ResourceContainer.hpp"
#include "vk_api/vk_raii.hpp"

namespace vren
{
    class Pipeline
    {
        friend class Shader;

    protected:
        VkPipelineBindPoint m_bind_point;
        std::vector<std::shared_ptr<Shader>> m_shaders;

        std::shared_ptr<vk_pipeline> m_pipeline;
        std::shared_ptr<vk_pipeline_layout> m_pipeline_layout;
        std::vector<std::shared_ptr<vk_descriptor_set_layout>> m_descriptor_set_layouts;

        explicit Pipeline(VkPipelineBindPoint bind_point);

    public:
        ~Pipeline() = default;

        VkDescriptorSetLayout descriptor_set_layout(uint32_t set_id) const { return m_descriptor_set_layouts.at(set_id)->get(); };

        void bind(VkCommandBuffer command_buffer) const;
        void push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags shader_stage, void const* data, uint32_t length, uint32_t offset = 0) const;
        void bind_descriptor_set(VkCommandBuffer command_buffer, uint32_t set_id, VkDescriptorSet set) const;

        void acquire_and_bind_descriptor_set(
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container,
            uint32_t descriptor_set_idx,
            std::function<void(VkDescriptorSet descriptor_set)> const& update_func
        );

        virtual void recreate() = 0;

    protected:
        void clear_vk_objects();
        void add_resources(ResourceContainer& resource_container) const;
    };
} // namespace vren
