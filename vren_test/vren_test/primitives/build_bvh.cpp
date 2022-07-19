#include <gtest/gtest.h>
#include <benchmark/benchmark.h>

#include <memory>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>
#include <fmt/format.h>

#include <vren/toolbox.hpp>
#include <vren/vk_helpers/misc.hpp>
#include <vren/pipeline/profiler.hpp>

#include "app.hpp"

// ------------------------------------------------------------------------------------------------
// Benchmark
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// Unit testing
// ------------------------------------------------------------------------------------------------

bool test_aabb_point_intersection(
    glm::vec3 const& min,
    glm::vec3 const& max,
    glm::vec3 const& point
)
{
    return
        point.x >= min.x && point.y >= min.y && point.z >= min.z &&
        point.x <= max.x && point.y <= max.y && point.z <= max.z;
}

void traverse_bvh_r(
    vren::bvh_node const* bvh,
    uint32_t offset,
    glm::vec3 const& point,
    std::vector<uint32_t>& result
)
{
    for (uint32_t node_idx = 0; node_idx < 32; node_idx++)
    {
        vren::bvh_node const& node = bvh[offset + node_idx];

        if (node.is_invalid())
        {
            continue;
        }

        if (test_aabb_point_intersection(node.m_min, node.m_max, point))
        {
            if (node.is_leaf())
            {
                result.push_back(offset + node_idx);
            }
            else
            {
                traverse_bvh_r(bvh, node.m_next, point, result);
            }
        }
    }
}

void traverse_bvh(
    vren::bvh_node const* bvh,
    uint32_t root_node_idx,
    glm::vec3 const& point,
    std::vector<uint32_t>& result
)
{
    vren::bvh_node const& root_node = bvh[root_node_idx];
    if (test_aabb_point_intersection(root_node.m_min, root_node.m_max, point))
    {
        traverse_bvh_r(bvh, root_node.m_next, point, result);
    }
}

void print_bvh(vren::bvh_node const* bvh, size_t length)
{
    for (size_t i = 0; i < length; i++)
    {
        fmt::print(fmt::fg(fmt::color::dark_slate_gray), "({}) ", i);

        fmt::color color = bvh[i].is_leaf() ? fmt::color::green : (bvh[i].is_invalid() ? fmt::color::red : fmt::color::white);
        fmt::print(fmt::fg(color), "{}, ", bvh[i].m_next);
        /*
        fmt::print(
            fmt::fg(color), "m:({:.1f},{:.1f},{:.1f}) M:({:.1f},{:.1f},{:.1f}), ",
            bvh[i].m_min.x, bvh[i].m_min.y, bvh[i].m_min.z,
            bvh[i].m_max.x, bvh[i].m_max.y, bvh[i].m_max.z
        );*/
    }
    fmt::print("\n");
}

void run_build_bvh_test(
    uint32_t leaf_count,
    uint32_t traversal_count,
    bool verbose
)
{
    vren::build_bvh& build_bvh = VREN_TEST_APP()->m_context.m_toolbox->m_build_bvh;

    uint32_t padded_leaf_count = vren::build_bvh::get_padded_leaf_count(leaf_count);
    std::vector<vren::bvh_node> cpu_buffer(padded_leaf_count);

    // Init
    std::random_device rd;
    std::mt19937 e2(rd());
    std::uniform_real_distribution<> dist(0, 100);

    for (uint32_t i = 0; i < cpu_buffer.size(); i++)
    {
        vren::bvh_node& node = cpu_buffer.at(i);
        if (i < leaf_count)
        {
            glm::vec3 pos(dist(e2), dist(e2), dist(e2));
            glm::vec3 ext(dist(e2), dist(e2), dist(e2));

            node.m_next = vren::bvh_node::k_leaf_node;
            node.m_min = pos - ext;
            node.m_max = pos + ext;
        }
        else
        {
            node.m_next = vren::bvh_node::k_invalid_node;
        }
    }

    // Copy to GPU buffer
    size_t gpu_buffer_length = vren::build_bvh::get_buffer_length(padded_leaf_count);
    vren::vk_utils::buffer gpu_buffer = vren::vk_utils::alloc_host_visible_buffer(
        VREN_TEST_APP()->m_context,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        gpu_buffer_length * sizeof(vren::bvh_node),
        true
    );

    vren::bvh_node* gpu_buffer_ptr = reinterpret_cast<vren::bvh_node*>(gpu_buffer.m_allocation_info.pMappedData);

    std::memcpy(gpu_buffer_ptr, cpu_buffer.data(), padded_leaf_count * sizeof(vren::bvh_node));

    // Build BVH GPU-side
    uint32_t root_node_idx;
    vren::vk_utils::immediate_graphics_queue_submit(VREN_TEST_APP()->m_context, [&](VkCommandBuffer command_buffer, vren::resource_container& resource_container)
    {
        VkBufferMemoryBarrier buffer_memory_barrier{
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .buffer = gpu_buffer.m_buffer.m_handle,
            .offset = 0,
            .size = cpu_buffer.size() * sizeof(vren::bvh_node)
        };
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, NULL, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

        build_bvh(
            command_buffer,
            resource_container,
            gpu_buffer,
            padded_leaf_count,
            &root_node_idx
        );
    });

    if (verbose)
    {
        print_bvh(gpu_buffer_ptr, gpu_buffer_length);
    }

    // Testing
    for (uint32_t i = 0; i < traversal_count; i++)
    {
        std::vector<uint32_t> cpu_intersections{};
        std::vector<uint32_t> gpu_intersections{};

        glm::vec3 point(dist(e2), dist(e2), dist(e2));

        // CPU linear search
        for (uint32_t node_idx = 0; node_idx < cpu_buffer.size(); node_idx++)
        {
            vren::bvh_node& node = cpu_buffer.at(node_idx);
            if (test_aabb_point_intersection(node.m_min, node.m_max, point))
            {
                cpu_intersections.push_back(node_idx);
            }
        }

        // GPU BVH-accelerated search
        traverse_bvh(gpu_buffer_ptr, root_node_idx, point, gpu_intersections);

        // Comparison
        if (verbose)
        {
            fmt::print("[build_bvh] BVH leaves: {}, test {} - CPU intersections: {}, GPU intersections: {}\n", leaf_count, i, cpu_intersections.size(), gpu_intersections.size());
        }
        
        ASSERT_EQ(cpu_intersections.size(), gpu_intersections.size());

        std::sort(cpu_intersections.begin(), cpu_intersections.end());
        std::sort(gpu_intersections.begin(), gpu_intersections.end());

        for (uint32_t j = 0; j < gpu_intersections.size(); j++)
        {
            ASSERT_EQ(cpu_intersections.at(j), gpu_intersections.at(j));
        }
    }
}

TEST(build_bvh, main)
{
    run_build_bvh_test(10, 16, false);
    run_build_bvh_test(100, 8, false);
    run_build_bvh_test(1000, 4, false);
    run_build_bvh_test(10000, 2, false);
    run_build_bvh_test(100000, 1, false);
}
