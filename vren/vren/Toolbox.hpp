#pragma once

#include <memory>

#include "primitives/blelloch_scan.hpp"
#include "primitives/bucket_sort.hpp"
#include "primitives/build_bvh.hpp"
#include "primitives/radix_sort.hpp"
#include "primitives/reduce.hpp"
#include "scene/material.hpp"
#include "scene/texture_manager.hpp"
#include "vk_helpers/image.hpp"
#include "wrappers/CommandPool.hpp"
#include "wrappers/DescriptorPool.hpp"

namespace vren
{
    class Toolbox
    {
    private:
        std::unique_ptr<CommandPool> m_command_pool;
        std::unique_ptr<DescriptorPool> m_descriptor_pool;

        vren::texture_manager m_texture_manager;

        // Reduce
        vren::reduce<glm::uint, vren::ReduceOperationAdd> m_reduce_uint_add;
        vren::reduce<glm::uint, vren::ReduceOperationMin> m_reduce_uint_min;
        vren::reduce<glm::uint, vren::ReduceOperationMax> m_reduce_uint_max;
        vren::reduce<glm::vec4, vren::ReduceOperationAdd> m_reduce_vec4_add;
        vren::reduce<glm::vec4, vren::ReduceOperationMin> m_reduce_vec4_min;
        vren::reduce<glm::vec4, vren::ReduceOperationMax> m_reduce_vec4_max;

        vren::blelloch_scan m_blelloch_scan;
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
