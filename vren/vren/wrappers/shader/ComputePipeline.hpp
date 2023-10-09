#pragma once

#include "Pipeline.hpp"

namespace vren
{
    class ComputePipeline : public Pipeline
    {
    private:
        std::shared_ptr<Shader> m_shader;

    public:
        explicit ComputePipeline(std::shared_ptr<Shader>& shader);
        ~ComputePipeline() = default;

        void dispatch(
            uint32_t workgroup_count_x,
            uint32_t workgroup_count_y,
            uint32_t workgroup_count_z,
            VkCommandBuffer command_buffer,
            ResourceContainer& resource_container
        ) const;

    protected:
        void recreate() override;
    };
} // namespace vren
