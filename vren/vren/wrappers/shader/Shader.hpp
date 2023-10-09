#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>
#include <volk.h>

#include "ShaderType.hpp"
#include "wrappers/DescriptorSetLayoutInfo.hpp"
#include "wrappers/HandleDeleter.hpp"

namespace vren
{
    // Forward decl
    class Pipeline;
    class ComputePipeline;
    class GraphicsPipeline;

    class Shader
    {
        friend class Pipeline;

    private:
        /// A list containing all the shaders ever created, allows to recompile all shaders.
        static std::vector<std::shared_ptr<Shader>> s_shaders;

        std::string m_filename;
        ShaderType m_type;

        shaderc::CompileOptions m_compile_options;

        std::shared_ptr<HandleDeleter<VkShaderModule>> m_handle;

        size_t m_push_constant_block_size;
        std::vector<DescriptorSetLayoutInfo> m_descriptor_set_layout_infos;

        /// The pipelines that depends on this shader.
        std::vector<std::weak_ptr<Pipeline>> m_dependant_pipelines;

        explicit Shader(std::string filename, ShaderType shader_type);

    public:
        ~Shader();

        std::string const& filename() const { return m_filename; }
        ShaderType type() const { return m_type; }

        VkShaderStageFlags vk_shader_stage() const;

        bool is_valid() const { return bool(m_handle); }

        bool has_push_constant_block() const { return m_push_constant_block_size == 0; };
        size_t push_constant_block_size() const { return m_push_constant_block_size; }
        std::span<DescriptorSetLayoutInfo> descriptor_set_layout_infos() const
        {
            return std::span(m_descriptor_set_layout_infos.begin(), m_descriptor_set_layout_infos.end());
        };

        void add_macro_definition(std::string const& name, std::string const& value);

        /// Discards the compiled shader module and recreates it, by reloading the GLSL source code from file.
        void recompile();

        static std::shared_ptr<Shader> create(std::string const& filename, ShaderType shader_type);
        static void recompile_all();

    private:
        void compact_dependant_pipelines();
    };
} // namespace vren
