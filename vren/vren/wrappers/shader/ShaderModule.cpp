#include "ShaderModule.hpp"

#include <stdexcept>

#include "shaderc/shaderc.hpp"

using namespace vren;
using namespace std::string_literals;

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

void ShaderModule::reload()
{
    std::ifstream in_file(m_filename);

    shaderc::Compiler glsl_compiler{};
    glsl_compiler.CompileGlslToSpv();
}
