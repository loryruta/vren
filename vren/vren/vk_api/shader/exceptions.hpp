#pragma once

#include <stdexcept>

namespace vren
{
    /// Exception thrown if the shader can't be compiled from GLSL code to SPIR-V.
    class ShaderCompilationException : std::exception
    {
    };

    /// Exception thrown if, after SPIR-V parsing, shader configuration results to be invalid (i.e. invalid descriptor sets, invalid push constant blocks, ...).
    class InvalidShaderException : std::exception
    {
    public:
        explicit InvalidShaderException(std::string const& message) :
            std::exception(message.c_str())
        {
        }
    };

} // namespace vren