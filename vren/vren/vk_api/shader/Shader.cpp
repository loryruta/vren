#include "Shader.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include "Context.hpp"
#include "Pipeline.hpp"
#include "exceptions.hpp"

using namespace vren;
using namespace std::string_literals;

namespace
{
    shaderc_shader_kind to_shaderc_kind(VkShaderStageFlags shader_stage)
    {
        switch (shader_stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return shaderc_shader_kind::shaderc_vertex_shader;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return shaderc_shader_kind::shaderc_fragment_shader;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            return shaderc_shader_kind::shaderc_compute_shader;
        default:
            throw std::runtime_error("Unsupported shader stage");
        }
    }

    VkDescriptorType to_vk_descriptor_type(SpvReflectDescriptorType spv_descriptor_type)
    {
        switch (spv_descriptor_type)
        {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:
            throw std::runtime_error("Unsupported SPIRV-Reflect descriptor type");
        }
    }

} // namespace

// ------------------------------------------------------------------------------------------------ ShaderRegistry

void ShaderRegistry::register_shader(std::shared_ptr<Shader> shader)
{
    m_shaders.emplace_back(std::move(shader));
}

void ShaderRegistry::unregister_shader(Shader& shader)
{
    auto iterator = m_shaders.begin();
    for (; iterator != m_shaders.end(); iterator++)
    {
        if (iterator->get() == &shader)
        {
            m_shaders.erase(iterator);
            break;
        }
    }
    assert(iterator != m_shaders.end());
}

void ShaderRegistry::recompile_all()
{
    for (std::shared_ptr<Shader> const& shader : m_shaders) shader->recompile();
}

// ------------------------------------------------------------------------------------------------ Shader

Shader::Shader(std::string filename, VkShaderStageFlags shader_stage) :
    m_filename(std::move(filename)),
    m_shader_stage(shader_stage)
{
    ShaderRegistry::get().register_shader(shared_from_this());

    recompile();
}

Shader::~Shader()
{
    ShaderRegistry::get().unregister_shader(*this);
}

void Shader::compact_dependant_pipelines()
{
    for (auto iterator = m_dependant_pipelines.begin(); iterator != m_dependant_pipelines.end(); iterator++)
    {
        if (iterator->expired())
            m_dependant_pipelines.erase(iterator);
    }
}

void Shader::load_spv_info()
{
    SpvReflectShaderModule spv_reflect_module;
    SpvReflectResult spv_reflect_result = spvReflectCreateShaderModule(m_spirv_code.size() * sizeof(uint32_t), m_spirv_code.data(), &spv_reflect_module);
    assert(spv_reflect_result == SPV_REFLECT_RESULT_SUCCESS); // Should always be valid if the GLSL compilation succeeded

    // Push constant blocks
    if (spv_reflect_module.push_constant_block_count > 0)
    {
        if (spv_reflect_module.push_constant_block_count == 1)
        {
            throw InvalidShaderException(
                std::format("Shader must have only one push constant block (found: {}): {}", spv_reflect_module.push_constant_block_count, m_filename)
            );
        }

        SpvReflectBlockVariable const& spv_block_variable = spv_reflect_module.push_constant_blocks[0];
        m_push_constant_block_size = spv_block_variable.padded_size;
    }

    // Descriptor set layouts
    for (int i = 0; i < spv_reflect_module.descriptor_set_count; i++)
    {
        SpvReflectDescriptorSet const& spv_descriptor_set = spv_reflect_module.descriptor_sets[i];

        DescriptorSetLayoutInfo descriptor_set_layout_info{};
        for (int j = 0; j < spv_descriptor_set.binding_count; j++)
        {
            SpvReflectDescriptorBinding const* spv_binding = spv_descriptor_set.bindings[j];

            DescriptorSetLayoutInfo::Binding binding_info{};
            binding_info.m_binding = spv_binding->binding;
            binding_info.m_descriptor_type = to_vk_descriptor_type(spv_binding->descriptor_type);
            binding_info.m_descriptor_count = spv_binding->count;
            descriptor_set_layout_info.m_bindings.push_back(binding_info);
        }

        m_descriptor_set_layout_info_map.emplace(spv_descriptor_set.set, descriptor_set_layout_info);
    }
}

void Shader::recompile()
{
    m_push_constant_block_size = 0;
    m_descriptor_set_layout_info_map.clear();

    m_handle.reset();

    // Load GLSL source code from the given filename
    std::ifstream in_file(m_filename);
    std::string glsl_code(std::istreambuf_iterator<char>{in_file}, {});

    // Compile GLSL to SPIR-V
    shaderc::Compiler glsl_compiler{};

    // TODO Play more with compile options:
    //   e.g. optimization level, include directories

    shaderc::CompilationResult<uint32_t> compilation_result =
        glsl_compiler.CompileGlslToSpv(glsl_code, to_shaderc_kind(m_shader_stage), m_filename.c_str(), m_compile_options);
    if (compilation_result.GetCompilationStatus() == shaderc_compilation_status::shaderc_compilation_status_success)
        throw ShaderCompilationException();

    m_spirv_code.resize(compilation_result.end() - compilation_result.begin());
    m_spirv_code.insert(m_spirv_code.begin(), compilation_result.begin(), compilation_result.end());

    // Reflect SPIR-V code to extract information such as push constants and descriptor set layouts
    load_spv_info();

    // Create shader module
    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = m_spirv_code.size();
    shader_info.pCode = m_spirv_code.data();

    VkShaderModule handle;
    VREN_CHECK(vkCreateShaderModule(Context::get().device().handle(), &shader_info, nullptr, &handle));
    m_handle = std::make_shared<vk_shader_module>(handle);

    // Free unneeded data
    m_glsl_code.clear();
    m_spirv_code.clear();

    // Trigger the recreation of the pipelines depending on this shader
    for (std::weak_ptr<Pipeline>& dependant_pipeline : m_dependant_pipelines)
    {
        if (std::shared_ptr<Pipeline> pipeline = dependant_pipeline.lock())
            pipeline->recreate();
    }
}

// ------------------------------------------------------------------------------------------------ ShaderBuilder

ShaderBuilder::ShaderBuilder(std::string const& filename, VkShaderStageFlags shader_stage) :
    m_shader(std::shared_ptr<Shader>(new Shader(filename, shader_stage)))
{
}

ShaderBuilder& ShaderBuilder::add_macro_definition(std::string const& name, std::string const& value)
{
    m_shader->m_compile_options.AddMacroDefinition(name, value);
    return *this;
}

std::shared_ptr<Shader>&& ShaderBuilder::build()
{
    return std::move(m_shader);
}
