#pragma once

#include <functional>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "base/base.hpp"
#include "base/resource_container.hpp"
#include "vk_raii.hpp"

namespace vren
{
    // Forward decl
    class context;

    // ------------------------------------------------------------------------------------------------

    static constexpr uint32_t k_max_variable_count_descriptor_count = 65536;

    // ------------------------------------------------------------------------------------------------
    // Shader module
    // ------------------------------------------------------------------------------------------------

    struct shader_module_entry_point
    {
        std::string m_name;
        VkShaderStageFlags m_shader_stage;
    };

    struct shader_module_binding_info
    {
        VkDescriptorType m_descriptor_type;
        uint32_t m_descriptor_count; // Count 0 means it's a variable descriptor count

        inline bool is_variable_descriptor_count() const { return m_descriptor_count == 0; }
    };

    using shader_module_descriptor_set_layout_info_t = std::unordered_map<uint32_t, shader_module_binding_info>;

    // void print_descriptor_set_layouts(std::unordered_map<uint32_t, vren::shader_module_descriptor_set_layout_info_t>
    // const& descriptor_set_layouts);

    struct shader_module
    {
        std::string m_name;

        vren::vk_shader_module m_handle;

        std::vector<shader_module_entry_point> m_entry_points;
        std::unordered_map<uint32_t, vren::shader_module_descriptor_set_layout_info_t> m_descriptor_set_layouts;
        size_t m_push_constant_block_size;

        std::vector<std::string> m_specialization_constant_names;
        std::vector<VkSpecializationMapEntry> m_specialization_map_entries;

        inline shader_module_entry_point const& get_entry_point(std::string const& name) const
        {
            return vren::find_if_or_fail_const(
                m_entry_points.begin(),
                m_entry_points.end(),
                [&](vren::shader_module_entry_point const& element)
                {
                    return element.m_name == name;
                }
            );
        }

        inline bool has_push_constant_block() const { return m_push_constant_block_size > 0; }

        inline uint32_t get_specialization_constant_id(std::string const& constant_name) const
        {
            auto found = std::find(
                m_specialization_constant_names.begin(), m_specialization_constant_names.end(), constant_name
            );
            if (found == m_specialization_constant_names.end())
            {
                throw std::runtime_error("Failed to find specialization constant by name");
            }

            return m_specialization_map_entries.at(found - m_specialization_constant_names.begin()).constantID;
        }
    };

    vren::shader_module load_shader_module(
        vren::context const& context, uint32_t const* code, size_t code_length, char const* name = "untitled"
    );
    vren::shader_module load_shader_module_from_file(vren::context const& context, char const* filename);

    // ------------------------------------------------------------------------------------------------
    // Specialized shader
    // ------------------------------------------------------------------------------------------------

    class specialized_shader
    {
    private:
        vren::shader_module const* m_shader_module;
        vren::shader_module_entry_point const* m_entry_point;

        std::vector<uint8_t> m_specialization_data;

    public:
        inline specialized_shader(vren::shader_module const& shader_module, std::string const& entry_point = "main") :
            m_shader_module(&shader_module),
            m_entry_point(&shader_module.get_entry_point(entry_point))
        {
            size_t specialization_data_size = 0;
            for (VkSpecializationMapEntry const& specialization_map_entry : shader_module.m_specialization_map_entries)
            {
                specialization_data_size += specialization_map_entry.size;
            }
            m_specialization_data.resize(specialization_data_size);
        }

        specialized_shader(vren::specialized_shader const& other) = delete;

        inline specialized_shader(vren::specialized_shader const&& other) :
            m_shader_module(other.m_shader_module),
            m_entry_point(other.m_entry_point),
            m_specialization_data(std::move(other.m_specialization_data))
        {
        }

        inline vren::shader_module const& get_shader_module() const { return *m_shader_module; }

        inline char const* get_entry_point() const { return m_entry_point->m_name.c_str(); }

        inline VkShaderStageFlags get_shader_stage() const { return m_entry_point->m_shader_stage; }

        inline void const* get_specialization_data() const { return m_specialization_data.data(); }

        inline size_t get_specialization_data_length() const { return m_specialization_data.size(); }

        inline bool has_specialization_data() const { return m_specialization_data.size() > 0; }

        inline void set_specialization_data(uint32_t constant_id, void const* data, size_t size)
        {
            size_t offset = 0;
            for (VkSpecializationMapEntry const& specialization_map_entry :
                 m_shader_module->m_specialization_map_entries)
            {
                if (specialization_map_entry.constantID == constant_id)
                {
                    assert(size == specialization_map_entry.size);

                    memcpy(&m_specialization_data.data()[offset], data, specialization_map_entry.size);
                    return;
                }

                offset += specialization_map_entry.size;
            }

            throw std::runtime_error("Failed to find specialization constant for the given ID");
        }

        inline void set_specialization_data(std::string const& constant_name, void const* data, size_t size)
        {
            set_specialization_data(m_shader_module->get_specialization_constant_id(constant_name), data, size);
        }
    };

    // ------------------------------------------------------------------------------------------------
    // Pipeline
    // ------------------------------------------------------------------------------------------------

    struct pipeline
    {
        vren::context const* m_context;

        std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
        vren::vk_pipeline_layout m_pipeline_layout;
        vren::vk_pipeline m_pipeline;

        VkPipelineBindPoint m_bind_point;

        ~pipeline();

        void bind(VkCommandBuffer command_buffer) const;
        void bind_vertex_buffer(
            VkCommandBuffer command_buffer, uint32_t binding, VkBuffer vertex_buffer, VkDeviceSize offset = 0
        ) const;
        void bind_index_buffer(
            VkCommandBuffer command_buffer, VkBuffer index_buffer, VkIndexType index_type, VkDeviceSize offset = 0
        ) const;
        void bind_descriptor_set(
            VkCommandBuffer command_buffer, uint32_t descriptor_set_idx, VkDescriptorSet descriptor_set
        ) const;
        void push_constants(
            VkCommandBuffer command_buffer,
            VkShaderStageFlags shader_stage,
            void const* data,
            uint32_t length,
            uint32_t offset = 0
        ) const;

        void acquire_and_bind_descriptor_set(
            vren::context const& context,
            VkCommandBuffer command_buffer,
            vren::resource_container& resource_container,
            uint32_t descriptor_set_idx,
            std::function<void(VkDescriptorSet descriptor_set)> const& update_func
        );

        void dispatch(
            VkCommandBuffer command_buffer,
            uint32_t workgroup_count_x,
            uint32_t workgroup_count_y,
            uint32_t workgroup_count_z
        ) const;
    };

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
