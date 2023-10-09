#pragma once

#include <cstdint>
#include <vector>

namespace vren
{
    class ShaderSpvParser
    {
    public:
        ShaderSpvParser() = default;
        ~ShaderSpvParser() = default;

        /// Given a SPIR-V file, parses it and returns a ShaderInfo object.
        ShaderInfo parse(std::vector<uint32_t> const& spirv_code);
    };

} // namespace vren
