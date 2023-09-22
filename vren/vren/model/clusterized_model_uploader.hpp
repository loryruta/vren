#pragma once

#include "clusterized_model.hpp"
#include "clusterized_model_draw_buffer.hpp"

namespace vren
{
    // Forward decl
    class context;

    class clusterized_model_uploader
    {
    public:
        vren::clusterized_model_draw_buffer
        upload(vren::context const& context, vren::clusterized_model const& clusterized_model);
    };
} // namespace vren
