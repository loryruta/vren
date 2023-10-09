#include "ShaderSpvParser.hpp"

#include <cassert>
#include <spirv_reflect.h>
#include <stdexcept>

using namespace vren;

namespace
{
}

ShaderInfo ShaderSpvParser::parse(std::vector<uint32_t> const& spirv_code)
{
    ShaderInfo info{};



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
