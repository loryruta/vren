#include "buffer.hpp"

#include "Context.hpp"

using namespace vren;

Buffer::Buffer(VkBuffer buffer, VmaAllocation allocation) :
    m_buffer(buffer),
    m_allocation(allocation)
{
}

VmaAllocationInfo Buffer::vma_allocation_info() const
{
    VmaAllocationInfo allocation_info{};
    vmaGetAllocationInfo(Context::get().vma_allocator(), m_allocation.get(), &allocation_info);
    return allocation_info;
}

void* Buffer::mapped_pointer() const
{
    return vma_allocation_info().pMappedData;
}
