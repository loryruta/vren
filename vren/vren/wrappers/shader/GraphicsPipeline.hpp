#pragma once

#include "Pipeline.hpp"

namespace vren
{
    class GraphicsPipeline : public Pipeline
    {
    private:
        explicit GraphicsPipeline();
        ~GraphicsPipeline() = default;
    };

    // TODO builder class to create the graphics pipeline
}