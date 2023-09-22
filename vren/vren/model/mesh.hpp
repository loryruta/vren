#pragma once

#include <cstdint>

#include "base/base.hpp"
#include "gpu_repr.hpp"

namespace vren
{
    inline constexpr size_t k_max_meshlet_vertex_count = 64;
    inline constexpr size_t k_max_meshlet_primitive_count = 124;

    size_t clusterize_mesh(
        float const* vertices,
        size_t vertex_stride,
        uint32_t const* indices,
        size_t index_count,
        uint32_t* meshlet_vertices,
        uint8_t* meshlet_triangles,
        vren::meshlet* meshlets
    );
} // namespace vren
