#pragma once

#include "Pipeline.hpp"

namespace vren
{
    class ComputePipeline final : public Pipeline
    {
    public:
        explicit ComputePipeline(std::shared_ptr<Shader> shader);
        ~ComputePipeline() = default;

        void dispatch(
            uint32_t num_workgroups_x,
            uint32_t num_workgroups_y,
            uint32_t num_workgroups_z,
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container
        ) const;

    protected:
        void recreate() override;
    };
} // namespace vren
