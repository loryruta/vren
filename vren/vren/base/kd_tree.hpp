#pragma once

#include <cstdint>
#include <functional>

namespace vren
{
    // ------------------------------------------------------------------------------------------------
    // KD-tree
    // ------------------------------------------------------------------------------------------------

    struct kd_tree_node
    {
        union
        {
            float m_split;
            uint32_t m_index;
        };

        uint32_t m_axis : 2;
        union
        {
            uint32_t m_right_child_distance : 30;
            uint32_t m_leaf_count : 30;
        };
    };

    size_t kd_tree_build(
        float const* points,
        size_t point_stride,
        uint32_t* indices,
        size_t count,
        kd_tree_node* kd_tree,
        size_t node_offset,
        size_t max_leaf_point_count
    );

    using kd_tree_search_filter_t = std::function<bool(uint32_t point)>;

    const kd_tree_search_filter_t k_kd_tree_default_search_filter = [](uint32_t point) -> bool
    {
        return true;
    };

    void kd_tree_search(
        float const* points,
        size_t point_stride,
        uint32_t const* indices,
        size_t count,
        kd_tree_node* kd_tree,
        size_t node_offset,
        float const* sample,
        kd_tree_search_filter_t const& filter_predicate,
        uint32_t& best_point,
        float& best_distance_squared
    );
} // namespace vren
