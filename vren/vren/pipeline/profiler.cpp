#include "profiler.hpp"

#include <stdexcept>

#include "Context.hpp"
#include "vk_api/misc_utils.hpp"

vren::profiler::profiler(vren::context const& context) :
    m_context(&context),
    m_query_pool(create_query_pool())
{
}

vren::profiler::~profiler() {}

vren::vk_query_pool vren::profiler::create_query_pool() const
{
    VkQueryPoolCreateInfo query_pool_info{
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = NULL,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = k_max_slot_count * 2,
        .pipelineStatistics = NULL};

    VkQueryPool query_pool;
    VREN_CHECK(vkCreateQueryPool(m_context->m_device, &query_pool_info, nullptr, &query_pool), m_context);
    return vren::vk_query_pool(*m_context, query_pool);
}

void vren::profiler::profile(
    VkCommandBuffer command_buffer,
    vren::ResourceContainer& resource_container,
    uint32_t slot_idx,
    std::function<void(VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)> const& sample_func
)
{
    vkCmdResetQueryPool(command_buffer, m_query_pool.m_handle, slot_idx * 2, 2);
    vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 0);

    sample_func(command_buffer, resource_container);

    vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 1);
}

vren::render_graph_t
vren::profiler::profile(vren::render_graph_allocator& allocator, vren::render_graph_t const& sample, uint32_t slot_idx)
{
    auto head = allocator.allocate();
    head->set_name("profiler_start");
    head->set_callback(
        [=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
        {
            vkCmdResetQueryPool(command_buffer, m_query_pool.m_handle, slot_idx * 2, 2);
            vkCmdWriteTimestamp(
                command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 0
            );
        }
    );

    for (auto node_idx : vren::render_graph_get_start(allocator, sample))
    {
        head->add_next(node_idx);
    }

    auto tail = allocator.allocate();
    tail->set_name("profiler_end");
    tail->set_callback(
        [=](uint32_t frame_idx, VkCommandBuffer command_buffer, vren::ResourceContainer& resource_container)
        {
            vkCmdWriteTimestamp(
                command_buffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, m_query_pool.m_handle, slot_idx * 2 + 1
            );
        }
    );

    for (auto node_idx : vren::render_graph_get_end(allocator, sample))
    {
        allocator.get_node_at(node_idx)->add_next(tail);
    }

    return vren::render_graph_gather(head);
}

bool vren::profiler::read_timestamps(uint32_t slot_idx, uint64_t& start_timestamp, uint64_t& end_timestamp)
{
    uint64_t timestamps[2];
    VkResult result = vkGetQueryPoolResults(
        m_context->m_device,
        m_query_pool.m_handle,
        slot_idx * 2,
        2,
        sizeof(uint64_t) * 2,
        &timestamps[0],
        sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT
    );

    // TODO USE IT (assumed 1): auto t_period = m_context->m_physical_device_properties.limits.timestampPeriod;
    // TODO USE IT (assumed 64): auto t_valid_bits =
    // vren::vk_utils::get_queue_families_properties(m_context->m_physical_device).at(m_context->m_queue_families.m_graphics_idx).timestampValidBits;

    if (result == VK_NOT_READY)
    {
        return false;
    }
    else if (result == VK_SUCCESS)
    {
        start_timestamp = timestamps[0];
        end_timestamp = timestamps[1];
        return true;
    }
    else
    {
        throw std::runtime_error("Failed to read query results");
    }
}
