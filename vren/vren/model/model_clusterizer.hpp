#pragma once

#include "clusterized_model.hpp"
#include "model.hpp"

namespace vren
{
    class model_clusterizer
    {
    private:
        void reserve_buffer_space(vren::clusterized_model& output, vren::model const& model);
        void clusterize_mesh(vren::clusterized_model& output, vren::model const& model, vren::model::mesh const& mesh);
        void clusterize_model(vren::clusterized_model& output, vren::model const& model);

    public:
        vren::clusterized_model clusterize(vren::model const& model);
    };
} // namespace vren
