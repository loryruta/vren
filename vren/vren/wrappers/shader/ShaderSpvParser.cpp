#include "ShaderSpvParser.hpp"

#include <cassert>
#include <spirv_reflect.h>
#include <stdexcept>

using namespace vren;

namespace
{
    ShaderInfo::EntryPoint parse_entry_point(SpvReflectEntryPoint const& entry_point)
    {
        ShaderInfo::EntryPoint info{};
        info.m_name = entry_point.name;
        info.m_shader_stage = static_cast<VkShaderStageFlagBits>(entry_point.shader_stage);
        return info;
    }
}

ShaderInfo ShaderSpvParser::parse(std::vector<uint32_t> const& spirv_code)
{
    ShaderInfo info{};

    SpvReflectShaderModule spv_reflect_module;
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_code.size() * sizeof(uint32_t), spirv_code.data(), &spv_reflect_module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (int i = 0; i < spv_reflect_module.entry_point_count; i++)
    {
        SpvReflectEntryPoint const& entry_point = spv_reflect_module.entry_points[i];
        info.m_entry_points.push_back(parse_entry_point(entry_point));
    }

    if (spv_reflect_module.push_constant_block_count > 0)
    {
        if (spv_reflect_module.push_constant_block_count > 1)
            throw std::runtime_error("Multiple push constant blocks are not supported");

        SpvReflectBlockVariable const& block_variable = spv_reflect_module.push_constant_blocks[0];
        info.m_push_constant_block_size = block_variable.padded_size;
    }
}
