#include "Shader.hpp"

#include <fstream>
#include <shaderc/shaderc.hpp>
#include <stdexcept>
#include <utility>

#include "Context.hpp"
#include "Pipeline.hpp"
#include "exceptions.hpp"

using namespace vren;
using namespace std::string_literals;

namespace
{
    shaderc_shader_kind to_shaderc_kind(ShaderType shader_type)
    {
        switch (shader_type)
        {
        case ShaderType::Vertex:
            return shaderc_shader_kind::shaderc_vertex_shader;
        case ShaderType::Fragment:
            return shaderc_shader_kind::shaderc_fragment_shader;
        case ShaderType::Compute:
            return shaderc_shader_kind::shaderc_compute_shader;
        default:
            throw std::runtime_error("Invalid shader type");
        }
    }

    VkShaderStageFlags to_shader_stage(ShaderType shader_type)
    {
        switch (shader_type)
        {
        case ShaderType::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderType::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderType::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            throw std::runtime_error("Invalid shader type");
        }
    }
} // namespace

Shader::Shader(std::string filename, ShaderType shader_type) :
    m_filename(std::move(filename)),
    m_type(shader_type)
{
    recompile();
}

Shader::~Shader()
{
    // Once destructed, unregister the shader from the global shaders list
    auto iterator = s_shaders.begin();
    for (; iterator != s_shaders.end(); iterator++)
    {
        if (iterator->get() == this)
        {
            s_shaders.erase(iterator);
            break;
        }
    }
    assert(iterator != s_shaders.end());
}

void Shader::add_macro_definition(std::string const& name, std::string const& value)
{
    m_compile_options.AddMacroDefinition(name, value);
}

VkShaderStageFlags Shader::vk_shader_stage() const
{
    return to_shader_stage(m_type);
}

void Shader::compact_dependant_pipelines()
{
    for (auto iterator = m_dependant_pipelines.begin(); iterator != m_dependant_pipelines.end(); iterator++)
    {
        if (iterator->expired())
            m_dependant_pipelines.erase(iterator);
    }
}

void Shader::recompile()
{
    m_handle.reset();

    // Load GLSL source code from the given filename
    std::ifstream in_file(m_filename);
    std::string glsl_code(std::istreambuf_iterator<char>{in_file}, {});

    // Compile GLSL to SPIR-V
    shaderc::Compiler glsl_compiler;

    // TODO Play more with compile options:
    //   e.g. optimization level, include directories

    shaderc::CompilationResult<uint32_t> compilation_result =
        glsl_compiler.CompileGlslToSpv(glsl_code, to_shaderc_kind(m_type), m_filename.c_str(), m_compile_options);
    if (compilation_result.GetCompilationStatus() == shaderc_compilation_status::shaderc_compilation_status_success)
        throw ShaderCompilationException();

    uint32_t const* spirv_code = compilation_result.begin();
    size_t spirv_code_size = compilation_result.end() - compilation_result.begin();

    // Create shader module
    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = spirv_code_size;
    shader_info.pCode = spirv_code;

    VkShaderModule handle;
    VREN_CHECK(vkCreateShaderModule(Context::get().device().handle(), &shader_info, nullptr, &handle));
    m_handle = std::make_shared<HandleDeleter<VkShaderModule>>(handle);

    // Trigger the recreation of the pipelines depending on this shader
    for (std::weak_ptr<Pipeline>& dependant_pipeline : m_dependant_pipelines)
    {
        if (std::shared_ptr<Pipeline> pipeline = dependant_pipeline.lock())
            pipeline->recreate();
    }
}

std::shared_ptr<Shader> Shader::create(std::string const& filename, ShaderType shader_type)
{
    std::shared_ptr<Shader> shader = std::shared_ptr<Shader>(new Shader(filename, shader_type));
    s_shaders.emplace_back(shader);
    return shader;
}

void Shader::recompile_all()
{
    for (std::shared_ptr<Shader> const& shader : s_shaders)
        shader->recompile();
}
