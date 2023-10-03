#include "ShaderModule.hpp"

#include <fstream>
#include <stdexcept>

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>
#include "Context.hpp"

using namespace vren;
using namespace std::string_literals;
using namespace shaderc;

ShaderModule::EntryPoint const& ShaderModule::entry_point(std::string const& name) const
{
    return vren::find_if_or_fail_const(
        m_entry_points.begin(), m_entry_points.end(), [&](ShaderModule::EntryPoint const& entry_point) { return entry_point.m_name == name; }
    );
}

VkSpecializationMapEntry const& ShaderModule::specialization_constant(std::string const& name) const
{
    auto found = std::find(m_specialization_constant_names.begin(), m_specialization_constant_names.end(), name);
    if (found == m_specialization_constant_names.end())
        throw std::runtime_error("Can't find specialization constant: "s + name);
    return m_specialization_map_entries.at(found - m_specialization_constant_names.begin());
}

void ShaderModule::recompile()
{
    std::ifstream in_file(m_filename);
    std::string glsl_src((std::istreambuf_iterator<char>(in_file)), std::istreambuf_iterator<char>());

    Compiler glsl_compiler{};
    CompileOptions options{};
    // TODO include .vren directory, shader's parent directory, and any directory given by the user
    CompilationResult<uint32_t> compilation_result =
        glsl_compiler.CompileGlslToSpv(glsl_src, shaderc_shader_kind::shaderc_vertex_shader, m_filename.c_str());

    shaderc_compilation_status compilation_status = compilation_result.GetCompilationStatus();
    if (compilation_status != shaderc_compilation_status::shaderc_compilation_status_success)
        throw ShaderCompilationException();

    uint32_t* spirv_code = compilation_result.begin();
    size_t spirv_code_size = compilation_result.end() - compilation_result.begin();

    VkShaderModuleCreateInfo shader_info{};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = spirv_code_size;
    shader_info.pCode = spirv_code;

    VkShaderModule handle;
    VREN_CHECK(vkCreateShaderModule(Context::get().device().handle(), &shader_info, nullptr, &handle));

    SpvReflectShaderModule module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_code_size * sizeof(uint32_t), spirv_code, &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);


}
