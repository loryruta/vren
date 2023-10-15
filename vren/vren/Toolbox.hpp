#pragma once

#include <memory>

#include "primitives/BlellochScan.hpp"
#include "primitives/Reduce.hpp"
#include "primitives/bucket_sort.hpp"
#include "primitives/build_bvh.hpp"
#include "primitives/radix_sort.hpp"
#include "scene/material.hpp"
#include "scene/texture_manager.hpp"
#include "vk_api/image/utils.hpp"

namespace vren
{
    class Toolbox // TODO don't like this class, delete it in favor of singletons
    {
    private:
        std::unique_ptr<CommandPool> m_command_pool;
        std::unique_ptr<DescriptorPool> m_descriptor_pool;

        vren::texture_manager m_texture_manager;

        // Reduce
        vren::Reduce<glm::uint, vren::ReduceOperationAdd> m_reduce_uint_add;
        vren::Reduce<glm::uint, vren::ReduceOperationMin> m_reduce_uint_min;
        vren::Reduce<glm::uint, vren::ReduceOperationMax> m_reduce_uint_max;
        vren::Reduce<glm::vec4, vren::ReduceOperationAdd> m_reduce_vec4_add;
        vren::Reduce<glm::vec4, vren::ReduceOperationMin> m_reduce_vec4_min;
        vren::Reduce<glm::vec4, vren::ReduceOperationMax> m_reduce_vec4_max;

        BlellochScan m_blelloch_scan;
        vren::radix_sort m_radix_sort;
        vren::bucket_sort m_bucket_sort;

        vren::build_bvh m_build_bvh;

    public:
        explicit Toolbox();
        Toolbox(Toolbox const& other) = delete;
        ~Toolbox();

        static Toolbox& get();

    private:
        void init_descriptor_pool();
    };
} // namespace vren
