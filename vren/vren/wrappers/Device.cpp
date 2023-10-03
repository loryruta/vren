#include "Device.hpp"

#include "Context.hpp"

using namespace vren;

Device::Device(VkDevice handle, int num_queues) :
    m_handle(std::move(handle)),
    m_num_queues(num_queues)
{
}

std::span<VkQueue> Device::queues()
{
    if (m_queues.size() < m_num_queues)
    {
        m_queues.resize(m_num_queues);
        for (int i = 0; i < m_num_queues; i++)
            vkGetDeviceQueue(m_handle.get(), Context::get().m_queue_family_idx, i, &m_queues.at(i));
    }
    return m_queues;
}

void Device::clear_cached_data()
{
    m_queues.clear();
}
