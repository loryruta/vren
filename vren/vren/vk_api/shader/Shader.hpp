#pragma once

#include <memory>
#include <span>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>
#include <volk.h>

#include "vk_api/vk_raii.hpp"
#include "vk_api/DescriptorSetLayoutInfo.hpp"

namespace vren
{
    // Forward decl
    class Pipeline;
    class Shader;
    class ShaderBuilder;

    // ------------------------------------------------------------------------------------------------ ShaderRegistry

    class ShaderRegistry
    {
    private:
        static ShaderRegistry s_instance;

        std::vector<std::shared_ptr<Shader>> m_shaders;

    public:
        explicit ShaderRegistry() = default;
        ~ShaderRegistry() = default;

        void register_shader(std::shared_ptr<Shader> shader);
        void unregister_shader(Shader& shader);

        void recompile_all();

        static ShaderRegistry& get() { return s_instance; }
    };

    // ------------------------------------------------------------------------------------------------ Shader

    class Shader : public std::enable_shared_from_this<Shader>
    {
        friend class ShaderBuilder;

    private:
        std::string m_filename;
        VkShaderStageFlags m_shader_stage;

        shaderc::CompileOptions m_compile_options;

        std::shared_ptr<vk_shader_module> m_handle;

        /* Variables initialized after SPIR-V reflection */

        size_t m_push_constant_block_size = 0;

        std::unordered_map<uint32_t, DescriptorSetLayoutInfo> m_descriptor_set_layout_info_map{};
        uint32_t m_max_descriptor_set_id = 0;

        /// The pipelines that depends on this shader.
        std::vector<std::weak_ptr<Pipeline>> m_dependant_pipelines;

        /* Temporary variables only used during the compilation process */

        std::string m_glsl_code{};
        std::vector<uint32_t> m_spirv_code{};

        explicit Shader(std::string filename, VkShaderStageFlags shader_stage);

    public:
        ~Shader();

        std::string const& filename() const { return m_filename; }
        VkShaderStageFlags shader_stage() const { return m_shader_stage; }

        VkShaderModule handle() const { return m_handle->get(); }

        bool has_push_constant_block() const { return m_push_constant_block_size == 0; };
        size_t push_constant_block_size() const { return m_push_constant_block_size; }

        bool has_descriptor_set(uint32_t set_id) const { return m_descriptor_set_layout_info_map.contains(set_id); }
        DescriptorSetLayoutInfo const& descriptor_set_layout_info(uint32_t set_id) const { return m_descriptor_set_layout_info_map.at(set_id); };
        uint32_t max_descriptor_set_id() const { return m_max_descriptor_set_id; }

        void recompile();

    private:
        void compact_dependant_pipelines();

        void load_spv_info();
    };

    // ------------------------------------------------------------------------------------------------ ShaderBuilder

    class ShaderBuilder
    {
    private:
        Shader m_shader;

    public:
        explicit ShaderBuilder(std::string const& filename, VkShaderStageFlags shader_stage);
        ~ShaderBuilder() = default;

        ShaderBuilder& add_macro_definition(std::string const& name, std::string const& value);

        Shader&& build();
    };

} // namespace vren
